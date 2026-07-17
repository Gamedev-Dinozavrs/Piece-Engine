#pragma once

#include <window/Window.h>
#include <core/Timestep.h>
#include <event/MouseEvent.h>
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Piece {

    enum class TextureSlot {
        Albedo = 0,
        Normal,
        Height,
        Roughness,
        AmbientOcclusion
    };

    struct QuadMaterialView {
        uint32_t id{0};
        std::string albedoPath;
        std::string normalPath;
        std::string heightPath;
        std::string roughnessPath;
        std::string ambientOcclusionPath;
    };

    struct PointLightSettings {
        glm::vec3 position{0.0f, 0.0f, 0.0f};
        float radius{1.0f};
        glm::vec3 color{1.0f, 1.0f, 1.0f};
        float intensity{1.0f};
    };

    struct LightingSettings {
        glm::vec3 directionalDirection{-0.4f, -1.0f, -0.2f};
        float directionalIntensity{1.2f};
        glm::vec3 directionalColor{1.0f, 0.98f, 0.9f};
        float specularStrength{1.0f};
        float specularShininessMin{8.0f};
        float specularShininessMax{128.0f};
        uint32_t pointLightCount{0};
        std::array<PointLightSettings, 4> pointLights{};
    };

    class Device;
    class RenderPass;
    class VulkanContext;

    class Renderer {
    public:
        static void Init(Window* window);
        static void Shutdown();
        static void WaitIdle();
        static void DrawFrame();
        static void Update(Timestep ts);
        static void OnWindowResize(uint32_t width, uint32_t height);
        static bool OnMouseScrolled(MouseScrolledEvent& event);

        static Device& GetDevice();
        static RenderPass& GetRenderPass();
        static VulkanContext& GetVulkanContext();
        static void SetSwapChainRecreatedCallback(const std::function<void()>& callback);
        static uint32_t CreateQuad(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
        static uint32_t CreateCube(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
        static void CreateQuadInView();
        static void CreateCubeInView();
        static std::vector<QuadMaterialView> GetQuadMaterials();
        static bool SetQuadTexturePath(uint32_t quadId, TextureSlot slot, const std::string& path);
        static LightingSettings GetLightingSettings();
        static void SetLightingSettings(const LightingSettings& settings);
        static bool CreatePointLightInView();
        static void ResetDirectionalLight();

    private:
        static void CreateCommandBuffers();
        static void CleanupSwapChain();
        static void RecreateSwapChain();
    };

} // namespace Piece
