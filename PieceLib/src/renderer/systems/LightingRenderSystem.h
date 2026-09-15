#pragma once

#include <scene/World.h>

namespace Piece {

struct RendererContext;
class Pipeline;

namespace LightingRenderSystem {

void Initialize(RendererContext& ctx);
void Shutdown(RendererContext& ctx);
void UpdatePerFrame(RendererContext& ctx, uint32_t frameIndex);
void RecordComposite(
    VkCommandBuffer commandBuffer,
    VkExtent2D extent,
    Pipeline& pipeline,
    VkPipelineLayout pipelineLayout,
    VkDescriptorSet compositeDescriptorSet,
    VkDescriptorSet globalDescriptorSet);
void RecordPresent(
    VkCommandBuffer commandBuffer,
    VkExtent2D extent,
    Pipeline& pipeline,
    VkPipelineLayout pipelineLayout,
    VkDescriptorSet compositeDescriptorSet,
    VkDescriptorSet globalDescriptorSet);

LightingSettings GetSettings();
void SetSettings(const LightingSettings& settings);

} // namespace LightingRenderSystem

} // namespace Piece