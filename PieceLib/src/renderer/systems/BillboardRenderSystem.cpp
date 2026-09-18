#include <PiecePCH.h>

#include <renderer/systems/BillboardRenderSystem.h>

#include <renderer/Pipeline.h>
#include <scene/Components.h>
#include <scene/Entity.h>
#include <scene/EditorCamera.h>
#include <scene/Mesh.h>
#include <scene/Scene.h>

namespace Piece {

namespace BillboardRenderSystem {

namespace {

constexpr float kPointLightIconSize = 0.4f;

}

void Record(const RendererContext& ctx, const FrameInfo& frameInfo) {
	if (!ctx.billboardPipeline || !ctx.quadMesh || !frameInfo.camera || !frameInfo.scene) {
		return;
	}
	if (ctx.pointLightIconDescriptorSet == VK_NULL_HANDLE) {
		return;
	}

	auto view = frameInfo.scene->GetAllEntitiesViewWith<TransformComponent, PointLightComponent>();
	if (view.begin() == view.end()) {
		return;
	}

	ctx.billboardPipeline->bind(frameInfo.commandBuffer);

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

	const glm::mat4 viewMatrix = frameInfo.camera->view();
	const glm::mat4 viewProj = frameInfo.camera->projection() * viewMatrix;
	// Camera-space right/up axes read directly off the view matrix's rotation block.
	const glm::vec3 cameraRight(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
	const glm::vec3 cameraUp(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		ctx.billboardPipelineLayout,
		0,
		1,
		&ctx.pointLightIconDescriptorSet,
		0,
		nullptr);

	ctx.quadMesh->bind(frameInfo.commandBuffer);

	for (auto entityHandle : view) {
		const auto& transform = view.get<TransformComponent>(entityHandle);
		const auto& light = view.get<PointLightComponent>(entityHandle);
		Entity entity{entityHandle, frameInfo.scene};
		const uint64_t uuid = entity.HasComponent<TagComponent>()
			? static_cast<uint64_t>(entity.GetComponent<TagComponent>().id)
			: 0;

		BillboardPushConstants push{};
		push.viewProj = viewProj;
		push.worldPositionSize = glm::vec4(transform.position, kPointLightIconSize);
		push.cameraRight = glm::vec4(cameraRight, 0.0f);
		push.cameraUp = glm::vec4(cameraUp, 0.0f);
		push.tint = glm::vec4(light.color, 1.0f);
		push.entityData.x = static_cast<int32_t>(uuid & 0xffffffffu);
		push.entityData.y = static_cast<int32_t>(uuid >> 32u);

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			ctx.billboardPipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(BillboardPushConstants),
			&push);

		ctx.quadMesh->draw(frameInfo.commandBuffer);
	}
}

} // namespace BillboardRenderSystem

} // namespace Piece
