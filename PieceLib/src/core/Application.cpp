#include <PiecePCH.h>
#include <core/Application.h>
#include <core/Input.h>
#include <renderer/Renderer.h>
#include <renderer/ImGuiLayer.h>
#include <window/PieceWindowGLFW.h>

namespace Piece {

    Application* Application::s_Instance = nullptr;

    Application::Application(const std::string& name) {

        PIECE_CORE_ASSERT(!s_Instance, "Application already exist!");
        s_Instance = this;

        /*
        *  In future if we need more than 1 window framework we include it's implementation here and use it as we need!
        */
        m_Window = CreateScope<PieceWindowGLFW>(WindowProperties(name));
        m_Window->SetEventCallback(PIECE_BIND_EVENT_FUNC(Application::OnEvent));
        Input::SetWindow(m_Window->GetNativeWindow());

        Renderer::Init(m_Window.get());
        m_ImGuiLayer = new ImGuiLayer(Renderer::GetDevice(), Renderer::GetRenderPass(), Renderer::GetVulkanContext());
        PushOverlay(m_ImGuiLayer);
        Renderer::SetSwapChainRecreatedCallback([this]() {
            if (m_ImGuiLayer) {
                m_ImGuiLayer->OnSwapChainRecreated(Renderer::GetRenderPass());
            }
        });
    }

    Application::~Application() {
        Renderer::SetSwapChainRecreatedCallback({});
        Renderer::WaitIdle();
        m_LayerStack.Clear();
        m_ImGuiLayer = nullptr;
        Renderer::Shutdown();
    }

    void Application::Run() {
        m_IsRunning = true;

        while (m_IsRunning) {

            float time = (float)glfwGetTime(); // platform dependent
            Timestep timestep = time - m_LastFrameTime;
            m_LastFrameTime = time;

            if (!m_IsMinimized) {
                for (Layer* layer : m_LayerStack) {
                    layer->OnUpdate(timestep);
                }

                m_ImGuiLayer->Begin();
                for (Layer* layer : m_LayerStack) {
                    layer->OnImGuiRender();
                }
                m_ImGuiLayer->End();

                Renderer::Update(timestep);
                Renderer::DrawFrame();
            }

            m_Window->OnUpdate();
        }
    }

    void Application::OnEvent(Event& event) {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowCloseEvent>(PIECE_BIND_EVENT_FUNC(Application::OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(PIECE_BIND_EVENT_FUNC(Application::OnWindowResize));
        dispatcher.Dispatch<MouseScrolledEvent>(PIECE_BIND_EVENT_FUNC(Application::OnMouseScrolled));

        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();) {
            if (event.IsHandled)
                break;
            (*--it)->OnEvent(event);
        }
    }

    void Application::PushLayer(Layer* layer) {
        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Application::PushOverlay(Layer* overlay) {
        m_LayerStack.PushOverlay(overlay);
        overlay->OnAttach();
    }

    bool Application::OnWindowClose(WindowCloseEvent& event) {
        m_IsRunning = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& event) {
        if (event.getWidth() == 0 || event.getHeight() == 0) {
            m_IsMinimized = true;
            return true;
        }
        else {
            m_IsMinimized = false;
            Renderer::OnWindowResize(event.getWidth(), event.getHeight());
        }
        return false;
    }

    bool Application::OnMouseScrolled(MouseScrolledEvent& event) {
        Renderer::OnMouseScrolled(event);
        return false;
    }

} // namespace Piece