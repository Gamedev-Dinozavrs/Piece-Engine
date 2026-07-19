#include <PiecePCH.h>

#include <renderer/systems/SceneRenderSystem.h>

#include <renderer/Pipeline.h>
#include <scene/Components.h>
#include <scene/Entity.h>
#include <scene/EditorCamera.h>
#include <scene/Mesh.h>
#include <scene/Scene.h>
#include <scene/World.h>
#include <unordered_map>

namespace Piece {

namespace SceneRenderSystem {

namespace {

float Halton(uint32_t index, uint32_t base) {
	float result = 0.0f;
	float f = 1.0f;
	uint32_t current = index;
	while (current > 0) {
		f /= static_cast<float>(base);
		result += f * static_cast<float>(current % base);
		current /= base;
	}
	return result;
}

glm::vec2 GetTaaJitter(const RendererContext& ctx) {
	if (World::GetEnvironmentSettings().aaTechnique != AATechnique::TAA || ctx.swapChainExtent.width == 0 || ctx.swapChainExtent.height == 0) {
		return glm::vec2(0.0f);
	}

	const uint32_t sampleIndex = static_cast<uint32_t>(ctx.currentFrame % 8u) + 1u;
	glm::vec2 jitter{
		Halton(sampleIndex, 2u) - 0.5f,
		Halton(sampleIndex, 3u) - 0.5f};
	return jitter / glm::vec2(static_cast<float>(ctx.swapChainExtent.width), static_cast<float>(ctx.swapChainExtent.height));
}

Entity FindEntityByUUID(Scene& scene, UUID uuid) {
	auto view = scene.GetAllEntitiesViewWith<TagComponent>();
	for (auto entityHandle : view) {
		if (view.get<TagComponent>(entityHandle).id == uuid) {
			return Entity{entityHandle, &scene};
		}
	}

	return {};
}

glm::mat4 GetWorldTransform(Scene& scene, Entity entity) {
	glm::mat4 local = entity.GetComponent<TransformComponent>().GetTransform();
	if (!entity.HasComponent<HierarchyComponent>()) {
		return local;
	}

	const UUID parentUuid = entity.GetComponent<HierarchyComponent>().parent;
	if (static_cast<uint64_t>(parentUuid) == 0) {
		return local;
	}

	Entity parent = FindEntityByUUID(scene, parentUuid);
	if (!parent) {
		return local;
	}

	return GetWorldTransform(scene, parent) * local;
}

} // namespace

void Record(const RendererContext& ctx, const FrameInfo& frameInfo) {
	frameInfo.pipeline->bind(frameInfo.commandBuffer);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(frameInfo.swapChainExtent.width);
	viewport.height = static_cast<float>(frameInfo.swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(frameInfo.commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = frameInfo.swapChainExtent;
	vkCmdSetScissor(frameInfo.commandBuffer, 0, 1, &scissor);

	if (!frameInfo.camera || !frameInfo.scene || !frameInfo.materialDescriptorSets) {
		return;
	}

	auto view = frameInfo.scene->GetAllEntitiesViewWith<TransformComponent, MeshRendererComponent>();
	for (auto entityHandle : view) {
		Entity entity{entityHandle, frameInfo.scene};
		glm::mat4 model = GetWorldTransform(*frameInfo.scene, entity);
		const auto& meshRenderer = view.get<MeshRendererComponent>(entityHandle);

		Ref<Mesh> mesh = meshRenderer.mesh;
		if (!mesh) {
			switch (meshRenderer.primitiveType) {
			case PrimitiveType::Quad:
				mesh = ctx.quadMesh;
				break;
			case PrimitiveType::Cube:
				mesh = ctx.cubeMesh;
				break;
			case PrimitiveType::Sphere:
				mesh = ctx.sphereMesh;
				break;
			default:
				break;
			}
		}

		if (!mesh) {
			continue;
		}

		glm::mat4 view = frameInfo.camera->view();
		glm::mat4 proj = frameInfo.camera->projection();
		const glm::vec2 jitter = GetTaaJitter(ctx);
		proj[2][0] += jitter.x * 2.0f;
		proj[2][1] += jitter.y * 2.0f;

		ScenePushConstants push{};
		push.mvp = proj * view * model;
		push.model = model;
		push.materialData.x = (meshRenderer.normalSource == NormalSource::Vertex) ? 1 : 0;
		auto flagsIt = ctx.objectMaterialFlags.find(static_cast<uint32_t>(entityHandle));
		push.materialData.y = (flagsIt != ctx.objectMaterialFlags.end()) ? static_cast<int>(flagsIt->second) : 0;

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			frameInfo.pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(ScenePushConstants),
			&push);

		if (frameInfo.globalDescriptorSet != VK_NULL_HANDLE) {
			vkCmdBindDescriptorSets(
				frameInfo.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				frameInfo.pipelineLayout,
				1,
				1,
				&frameInfo.globalDescriptorSet,
				0,
				nullptr);
		}

		auto dsIt = frameInfo.materialDescriptorSets->find(static_cast<uint32_t>(entityHandle));
		if (dsIt == frameInfo.materialDescriptorSets->end()) {
			continue;
		}

		VkDescriptorSet descriptorSet = dsIt->second;
		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			frameInfo.pipelineLayout,
			0,
			1,
			&descriptorSet,
			0,
			nullptr);

		mesh->bind(frameInfo.commandBuffer);
		mesh->draw(frameInfo.commandBuffer);
	}
}

} // namespace SceneRenderSystem

} // namespace Piece
