#pragma once

#include <window/Window.h>
#include <core/Timestep.h>
#include <event/MouseEvent.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <functional>
#include <core/UUID.h>

namespace Piece {

    class Device;
    class RenderPass;
    class VulkanContext;
    class EditorCamera;

    class Renderer {
    public:
        static void Init(Window* window);
        static void Shutdown();
        static void WaitIdle();
        static void DrawFrame();
        static void Update(Timestep ts);
        static void OnWindowResize(uint32_t width, uint32_t height);
        static bool OnMouseScrolled(MouseScrolledEvent& event);
        static UUID ReadEntityIdAtPixel(uint32_t x, uint32_t y);
        static EditorCamera& GetEditorCamera();
        // When active, the viewport renders from the active scene's primary CameraComponent instead
        // of the free-fly editor camera (falls back to the editor camera if no primary camera exists).
        static void SetGameCameraActive(bool active);

        static Device& GetDevice();
        static RenderPass& GetRenderPass();
        static VulkanContext& GetVulkanContext();
        static void SetSwapChainRecreatedCallback(const std::function<void()>& callback);
        static uint32_t CreateQuad(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
        static uint32_t CreateCube(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
        static uint32_t CreateSphere(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
        static void CreateQuadInView();
        static void CreateCubeInView();
        static void CreateSphereInView();
        static bool CreatePointLightInView();

    private:
        static void CreateCommandBuffers();
        static void CleanupSwapChain();
        static void RecreateSwapChain();
    };

} // namespace Piece
