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

		ScenePushConstants push{};
		push.mvp = proj * view * model;
		push.model = model;
		uint32_t materialId = meshRenderer.materialId;
		if (entity.HasComponent<MaterialComponent>()) {
			materialId = entity.GetComponent<MaterialComponent>().materialId;
		}
		const MaterialSurfaceFactors materialFactors = World::ResolveMaterialSurfaceFactors(materialId);
		push.materialFactors.x = materialFactors.roughnessFactor;
		push.materialFactors.y = materialFactors.metallicFactor;
		const MaterialColors materialColors = entity.HasComponent<MaterialComponent>()
			? entity.GetComponent<MaterialComponent>().colors
			: World::ResolveMaterialColors(materialId);
		push.baseColor = glm::vec4(materialColors.baseColor, 1.0f);
		push.emissiveColor = glm::vec4(materialColors.emissiveColor, materialColors.emissiveEnabled ? 1.0f : 0.0f);
		push.bloomParams = glm::vec4(
			std::max(materialColors.bloomThreshold, 0.0f),
			std::max(materialColors.bloomIntensity, 0.0f),
			std::max(materialColors.bloomRadius, 0.0f),
			materialColors.hdrBloomEnabled ? 1.0f : 0.0f);
		push.bloomFlags.x = (materialColors.emissiveEnabled && materialColors.emissiveBloomEnabled) ? 1.0f : 0.0f;
		push.materialData.x = (meshRenderer.normalSource == NormalSource::Vertex) ? 1 : 0;
		auto flagsIt = ctx.objectMaterialFlags.find(static_cast<uint32_t>(entityHandle));
		push.materialData.y = (flagsIt != ctx.objectMaterialFlags.end()) ? static_cast<int>(flagsIt->second) : 0;
		const uint64_t uuid = static_cast<uint64_t>(entity.GetComponent<TagComponent>().id);
		push.materialData.z = static_cast<int32_t>(uuid & 0xffffffffu);
		push.materialData.w = static_cast<int32_t>(uuid >> 32u);

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
