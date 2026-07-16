#include <renderer/SceneRenderSystem.h>

#include <renderer/Pipeline.h>
#include <scene/EditorCamera.h>
#include <scene/Mesh.h>
#include <scene/RenderObject.h>

namespace Piece {

namespace SceneRenderSystem {

void Record(const FrameInfo& frameInfo) {
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

    if (!frameInfo.camera || !frameInfo.renderObjects || !frameInfo.materialDescriptorSets) {
        return;
    }

    for (RenderObject* renderObject : *frameInfo.renderObjects) {
        if (!renderObject || !renderObject->mesh()) {
            continue;
        }

        glm::mat4 model = renderObject->modelMatrix();
        glm::mat4 view = frameInfo.camera->view();
        glm::mat4 proj = frameInfo.camera->projection();

        ScenePushConstants push{};
        push.mvp = proj * view * model;
        push.model = model;

        vkCmdPushConstants(
            frameInfo.commandBuffer,
            frameInfo.pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
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

        auto dsIt = frameInfo.materialDescriptorSets->find(renderObject->objectId());
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

        renderObject->mesh()->bind(frameInfo.commandBuffer);
        renderObject->mesh()->draw(frameInfo.commandBuffer);
    }
}

} // namespace SceneRenderSystem

} // namespace Piece
