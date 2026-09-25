#pragma once

// Core
#include <core/BackgroundService.h>
#include <core/Core.h>
#include <core/EditorConsoleSink.h>
#include <core/Input.h>
#include <core/KeyCodes.h>
#include <core/Log.h>
#include <core/MouseButtonCodes.h>
#include <core/Timestep.h>
#include <core/UUID.h>

// Events
#include <event/ApplicationEvent.h>
#include <event/Event.h>
#include <event/KeyEvent.h>
#include <event/MouseEvent.h>

// GUI
#include <GUI/ImGuiLayer.h>

// Window
#include <window/Window.h>

// Renderer
#include <renderer/Buffer.h>
#include <renderer/Descriptors.h>
#include <renderer/Device.h>
#include <renderer/FrameInfo.h>
#include <renderer/FrameResources.h>
#include <renderer/Pipeline.h>
#include <renderer/RenderPass.h>
#include <renderer/Renderer.h>
#include <renderer/RendererContext.h>
#include <renderer/RendererInternals.h>
#include <renderer/Shader.h>
#include <renderer/ShaderLibrary.h>
#include <renderer/Surface.h>
#include <renderer/SwapChain.h>
#include <renderer/Texture.h>
#include <renderer/VulkanContext.h>
#include <renderer/passes/CompositePass.h>
#include <renderer/passes/GeometryPass.h>
#include <renderer/passes/LightingPass.h>
#include <renderer/systems/LightingRenderSystem.h>
#include <renderer/systems/SceneRenderSystem.h>
#include <renderer/systems/BillboardRenderSystem.h>
#include <renderer/systems/UIRenderSystem.h>

// Scene
#include <scene/Camera.h>
#include <scene/Components.h>
#include <scene/EditorCamera.h>
#include <scene/Entity.h>
#include <scene/Mesh.h>
#include <scene/PrimitiveMeshData.h>
#include <scene/RenderObject.h>
#include <scene/Scene.h>
#include <scene/World.h>

// Scripting
#include <scripting/ScriptComponent.h>
#include <scripting/ScriptEngine.h>

// Extended GLM
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
