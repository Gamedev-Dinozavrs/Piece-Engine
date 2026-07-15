#pragma once

#include <window/Window.h>
#include <core/Timestep.h>
#include <event/MouseEvent.h>
#include <vector>

namespace Piece {

    class Renderer {
    public:
        static void Init(Window* window);
        static void Shutdown();
        static void DrawFrame();
        static void Update(Timestep ts);
        static void OnWindowResize(uint32_t width, uint32_t height);
        static bool OnMouseScrolled(MouseScrolledEvent& event);

    private:
        static void CreateRenderPass();
        static void CreateCommandBuffers();
        static void CreateSyncObjects();
        static void CleanupSwapChain();
        static void RecreateSwapChain();
    };

} // namespace Piece
