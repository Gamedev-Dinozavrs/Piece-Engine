//pch
#include <window/PieceWindowGLFW.h>

#include <event/ApplicationEvent.h>
#include <event/KeyEvent.h>
#include <event/MouseEvent.h>

namespace Piece {

	static bool s_GLFWInitialized = false;

	static void GLFWErrorCallback(int error, const char* description) {
		// log "GLFW Error ({0}): {1} ", error, description
	}

	PieceWindowGLFW::PieceWindowGLFW(const WindowProperties& props) {
		//glfwWindowHint(GLFW_SAMPLES, 4);
		Init(props);
	}

	PieceWindowGLFW::~PieceWindowGLFW() {
		Shutdown();
	}

	void PieceWindowGLFW::OnUpdate() {
		glfwPollEvents();
		glfwSwapBuffers(m_Window);
		//m_context->swapBuffers();
	}

	void PieceWindowGLFW::SetVSync(bool enabled) {
		if (enabled) {
			glfwSwapInterval(1);
		}
		else {
			glfwSwapInterval(0);
		}

		m_Data.IsVSyncEnabled = enabled;
	}

	bool PieceWindowGLFW::IsVSyncOnOrNot() const
	{
		return m_Data.IsVSyncEnabled;
	}

	void PieceWindowGLFW::Init(const WindowProperties& props) {
		m_Data.Name = props.name;
		m_Data.Width = props.width;
		m_Data.Height = props.height;

		// TODO: Log

		if (!s_GLFWInitialized) {
			int success = glfwInit();

			// TODO: assert on not success
			glfwSetErrorCallback(GLFWErrorCallback);
			s_GLFWInitialized = true;
		}

		m_Window = glfwCreateWindow(static_cast<int>(m_Data.Width),
									static_cast<int>(m_Data.Height),
									m_Data.Name.c_str(),
									nullptr,
									nullptr);

		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent wrEvent(width, height);
			data.CallbackFunc(wrEvent);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			switch (action) {
				case GLFW_PRESS: {
					KeyPressedEvent kpEvent(key, 0);
					data.CallbackFunc(kpEvent);
					break;
				}
				case GLFW_RELEASE: {
					KeyReleasedEvent krEvent(key);
					data.CallbackFunc(krEvent);
					break;
				}
				case GLFW_REPEAT: {
					KeyPressedEvent kpEvent(key, 1);
					data.CallbackFunc(kpEvent);
					break;
				}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent ktEvent(keycode);
			data.CallbackFunc(ktEvent);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action) {
				case GLFW_PRESS: {
					MouseButtonPressedEvent mbpEvent(button);
					data.CallbackFunc(mbpEvent);
					break;
				}
				case GLFW_RELEASE: {
					MouseButtonReleasedEvent mbrEvent(button);
					data.CallbackFunc(mbrEvent);
					break;
				}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double offsetX, double offsetY) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent msEvent((float)offsetX, (float)offsetY);
			data.CallbackFunc(msEvent);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent mmEvent((float)xPos, (float)yPos);
			data.CallbackFunc(mmEvent);
		});
	}

	void PieceWindowGLFW::Shutdown() {
		glfwDestroyWindow(m_Window);
	}

} // namespace Piece