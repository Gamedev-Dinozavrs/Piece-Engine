# Piece Engine

Piece Engine is a real-time 3D engine built with Vulkan and C++.
It is currently focused on a clean renderer architecture, scene workflow, and an editor-first iteration loop.

## Current Status

The project is in active development.
Core rendering and editor workflows are working, with several advanced features planned next.

## Implemented Features

### Rendering

- Vulkan-based renderer
- Deferred rendering pipeline
	- Geometry pass
	- Lighting/composite pass
	- Present pass
- Multi-target G-buffer-style outputs in the offscreen pipeline
	- World position + roughness
	- Albedo + AO
	- Normal + AO
	- Emissive
	- Depth
- Configurable anti-aliasing pipeline
	- Off
	- FXAA
	- MSAA (1x/2x/4x/8x depending on GPU support)
	- TAA
- Lighting controls
	- Directional lighting
	- Point lights
	- Specular controls
- Environment lighting controls (IBL toggles and maps)
	- Diffuse environment map
	- Specular environment map
	- Intensity and strength controls
- Texture system
	- GPU texture upload via staging buffers
	- Mipmap generation
	- Sampler + image view creation and binding
	- Per-material texture slots (albedo, normal, roughness, AO, emissive)

### Scene and Runtime

- ECS-style scene/entity workflow using EnTT
- Primitive spawning
	- Quad
	- Cube
	- Sphere
- Runtime/editor spawn helpers for primitives and point lights
- Entity hierarchy and parenting
- Material assignment and per-material texture paths
- Material surface controls
	- Roughness factor
	- Metallic factor
- Scene camera navigation controls

### Asset Support

- OBJ import
- glTF/glb import
- Imported model grouping workflows (preserve, group by material, merge)
- Imported hierarchy utilities

### Editor

- ImGui integration
- Docking-based editor UI workflow
- Scene hierarchy panel
- Content browser panel
- Material editing tools
- LookDev controls for lighting, environment, and AA
- Environment texture assignment tools
	- Diffuse environment map picker/clear
	- Specular environment map picker/clear
- Material texture assignment tools for albedo/normal/roughness/AO/emissive

### Tooling

- Shader compilation script for Vulkan shader pipeline

## Planned Roadmap

- Dynamic Rendering (Vulkan)
- Advanced material system
- GPU-driven rendering
- Compute-based post-processing
- Volumetric lighting and fog
- Real-time environment map updates
- Editor and tooling expansion
- Multithreaded renderer improvements
- Advanced debugging and profiling tools

## Build Requirements

- Vulkan SDK
- CMake 3.10+
- C++17 compiler/toolchain
- GPU with Vulkan support
- VS Code (workspace includes .vscode configuration and tasks)

## Build and Run

From repository root, configure once and build your target(s) with CMake:

1. Configure
	- cmake -S . -B build
2. Build targets
	- cmake --build build --config Debug --target <your_target>

VS Code is supported out of the box (see .vscode). You can use the included tasks:

- CMake Configure
- CMake Build
- Run PieceEditor

## Project Layout

- PieceLib/src/core: application, input, logging, background services
- PieceLib/src/renderer: Vulkan renderer, swapchain, passes, pipelines, systems
- PieceLib/src/scene: ECS scene, entities, components, world state
- PieceLib/src/assets: model importers and asset processing
- PieceEditor/src: editor UI and workflows
- PieceLib/src/shaders: shader sources and shader compile script

## Philosophy

Piece Engine aims to keep low-level graphics control without sacrificing structure.
The goal is practical Vulkan development with maintainable engine architecture.

## Contributing

Issues and pull requests are welcome.

## License

This project is licensed under the MIT License.
See [LICENSE](LICENSE).
