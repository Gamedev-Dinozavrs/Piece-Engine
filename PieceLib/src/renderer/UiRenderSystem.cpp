#include <renderer/UiRenderSystem.h>

#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"

namespace Piece {

namespace UiRenderSystem {

void Record(VkCommandBuffer commandBuffer) {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }

    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData != nullptr && drawData->CmdListsCount > 0) {
        ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
    }
}

} // namespace UiRenderSystem

} // namespace Piece
