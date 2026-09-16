#include <PiecePCH.h>

#include <renderer/systems/LightingRenderSystem.h>

#include <renderer/Buffer.h>
#include <renderer/Descriptors.h>
#include <renderer/Pipeline.h>
#include <renderer/RendererContext.h>
#include <renderer/SwapChain.h>
#include <scene/EditorCamera.h>
#include <scene/World.h>

#include <algorithm>
#include <array>

namespace Piece {

namespace {

constexpr uint32_t kMaxPointLights = 4;

struct PointLightUbo {
	glm::vec4 positionRadius{0.0f, 0.0f, 0.0f, 1.0f};
	glm::vec4 colorIntensity{1.0f, 1.0f, 1.0f, 1.0f};
};

struct LightingUbo {
	glm::vec4 cameraPosition{0.0f, 0.0f, 3.0f, 0.0f};
	glm::mat4 invViewProj{1.0f};
	glm::vec4 dirLightDirection{-0.4f, -1.0f, -0.2f, 0.0f};
	glm::vec4 dirLightColorIntensity{1.0f, 0.98f, 0.9f, 1.2f};
	PointLightUbo pointLights[kMaxPointLights]{};
	glm::ivec4 pointLightCount{0, 0, 0, 0};
	glm::vec4 specularParams{1.0f, 8.0f, 128.0f, 0.0f};
	glm::vec4 iblParams{0.0f, 1.0f, 1.0f, 8.0f};
};

LightingUbo BuildLightingUbo(const RendererContext& ctx) {
	LightingUbo ubo{};
	LightingSettings lighting = World::GetLightingSettings();

	if (ctx.camera) {
		ubo.cameraPosition = glm::vec4(ctx.camera->position(), 0.0f);
		const glm::mat4 view = ctx.camera->view();
		glm::mat4 proj = ctx.camera->projection();
		ubo.invViewProj = glm::inverse(proj * view);
	}

	ubo.dirLightDirection = glm::vec4(lighting.directionalDirection, 0.0f);
	ubo.dirLightColorIntensity = glm::vec4(
		lighting.directionalColor,
		lighting.directionalEnabled ? lighting.directionalIntensity : 0.0f);

	const uint32_t lightCount = std::min<uint32_t>(lighting.pointLightCount, kMaxPointLights);
	for (uint32_t i = 0; i < lightCount; ++i) {
		const auto& src = lighting.pointLights[i];
		ubo.pointLights[i].positionRadius = glm::vec4(src.position, src.radius);
		ubo.pointLights[i].colorIntensity = glm::vec4(src.color, src.intensity);
	}
	ubo.pointLightCount.x = static_cast<int>(lightCount);

	float minShininess = std::max(1.0f, lighting.specularShininessMin);
	float maxShininess = std::max(minShininess, lighting.specularShininessMax);
	EnvironmentSettings environment = World::GetEnvironmentSettings();
	ubo.specularParams = glm::vec4(lighting.specularStrength, minShininess, maxShininess, environment.ambientStrength);

	ubo.iblParams = glm::vec4(
		environment.enabled ? environment.intensity : 0.0f,
		environment.diffuseStrength,
		environment.specularStrength,
		8.0f);

	return ubo;
}

} // namespace

namespace LightingRenderSystem {

void Initialize(RendererContext& ctx) {
	ctx.globalSetLayout = DescriptorSetLayout::Builder(*ctx.deviceWrapper)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build();

	ctx.globalDescriptorPool = DescriptorPool::Builder(*ctx.deviceWrapper)
		.setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
		.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(SwapChain::MAX_FRAMES_IN_FLIGHT))
		.setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
		.build();

	ctx.globalUboBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
	ctx.globalDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

	for (size_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
		ctx.globalUboBuffers[i] = CreateScope<Buffer>(
			*ctx.deviceWrapper,
			sizeof(LightingUbo),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		ctx.globalUboBuffers[i]->map();

		bool allocated = ctx.globalDescriptorPool->allocateDescriptor(
			ctx.globalSetLayout->getDescriptorSetLayout(),
			ctx.globalDescriptorSets[i]);
		PIECE_CORE_ASSERT(allocated, "Failed to allocate global lighting descriptor set");

		VkDescriptorBufferInfo bufferInfo = ctx.globalUboBuffers[i]->descriptorInfo(sizeof(LightingUbo));
		DescriptorWriter(*ctx.globalSetLayout, *ctx.globalDescriptorPool)
			.writeBuffer(0, &bufferInfo)
			.overwrite(ctx.globalDescriptorSets[i]);
	}
}

void Shutdown(RendererContext& ctx) {
	ctx.globalDescriptorSets.clear();
	ctx.globalUboBuffers.clear();
	ctx.globalDescriptorPool.reset();
	ctx.globalSetLayout.reset();
}

void UpdatePerFrame(RendererContext& ctx, uint32_t frameIndex) {
	if (frameIndex >= ctx.globalUboBuffers.size() || frameIndex >= ctx.globalDescriptorSets.size()) {
		return;
	}

	LightingUbo ubo = BuildLightingUbo(ctx);
	auto& buffer = ctx.globalUboBuffers[frameIndex];
	buffer->write(&ubo, sizeof(LightingUbo));
	buffer->flush(sizeof(LightingUbo));
}

void RecordComposite(
	VkCommandBuffer commandBuffer,
	VkExtent2D extent,
	Pipeline& pipeline,
	VkPipelineLayout pipelineLayout,
	VkDescriptorSet compositeDescriptorSet,
	VkDescriptorSet globalDescriptorSet) {
	pipeline.bind(commandBuffer);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = extent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	std::array<VkDescriptorSet, 2> descriptorSets = {compositeDescriptorSet, globalDescriptorSet};
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		static_cast<uint32_t>(descriptorSets.size()),
		descriptorSets.data(),
		0,
		nullptr);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void RecordPresent(
	VkCommandBuffer commandBuffer,
	VkExtent2D extent,
	Pipeline& pipeline,
	VkPipelineLayout pipelineLayout,
	VkDescriptorSet compositeDescriptorSet,
	VkDescriptorSet globalDescriptorSet) {
	pipeline.bind(commandBuffer);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = extent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	std::array<VkDescriptorSet, 2> descriptorSets = {compositeDescriptorSet, globalDescriptorSet};
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		static_cast<uint32_t>(descriptorSets.size()),
		descriptorSets.data(),
		0,
		nullptr);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

LightingSettings GetSettings() {
	return World::GetLightingSettings();
}

void SetSettings(const LightingSettings& settings) {
	World::SetLightingSettings(settings);
}

} // namespace LightingRenderSystem

} // namespace Piece
