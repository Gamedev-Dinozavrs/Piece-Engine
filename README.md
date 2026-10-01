# Piece Engine

Piece Engine is a real-time 3D engine built with Vulkan and C++.
It is currently focused on a clean renderer architecture, scene workflow, and an editor-first iteration loop.

## Current Status

The project is in active development.
Core rendering, editor workflows, a C# gameplay scripting runtime, and a standalone shipped-game runner are all working, with several advanced features planned next.
Currently it supports only Windows OS.
The engine targets cross-platform support, and most core systems are already platform-agnostic.
Cross-platform support is currently in progress, with the main remaining gap being OS-specific file open dialog handling.

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
- MSAA pipeline
	- Off
	- MSAA (1x/2x/4x/8x depending on GPU support)
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
- In-game camera objects with Perspective/Orthographic projection (FOV/size, near/far clip, primary-camera flag)
- Play mode renders from the scene's primary camera, with an automatic fallback camera if none is set
- Default "Main Camera" + "Directional Light" auto-populated on new/empty scenes
- Skeletal animation (glTF/FBX skins, joint palettes, Animator component with clip blending/state playback)
- Scene serialization to YAML (`.piecescene`), including materials, lighting, cameras, and animation state

### Gameplay Scripting (C#)

- CoreCLR (.NET 8) hosted in-process via hostfxr - no Mono dependency
- `PieceEngine` C# API (`ScriptBase`, `Entity`, `Transform`, `KeyCode`) for Unity-style gameplay scripts
- Two-assembly architecture for hot reload:
	- `Piece.ScriptCore` - stable native/managed bridge, loaded once per process
	- `Piece.GameScripts` - user scripts, compiled on demand and loaded into a collectible `AssemblyLoadContext`
- Automatic recompile + reload on Play, on script creation, and via a "Recompile Scripts" menu action - no editor restart needed to see script changes
- Script component in the inspector uses a drag-and-drop asset slot (matches the Content Browser tile) instead of a free-text class name field
- "New Script" creates a ready-to-edit `ScriptBase` subclass and opens it directly in Visual Studio

### Asset Support

- OBJ import
- glTF/glb import
- FBX import, including Mixamo-style animation-only clip retargeting onto an existing skeleton
- Imported model grouping workflows (preserve, group by material, merge)
- Imported hierarchy utilities
- Reusable material assets (`.piece-material`) with albedo/normal/roughness/AO/metallic/emissive slots

### Editor

- ImGui integration
- Docking-based editor UI workflow
- Scene hierarchy panel with Transform/Camera/Script/Animator inspectors
- Content browser panel
	- Dual Assets and Scripts roots with folder navigation
	- Right-click Open/Rename/Delete context menu on files and folders
	- Drag-and-drop for models, materials, and scripts
- Console panel (dev console) with command history and built-in commands (`help`, `clear`, `scene.info`, `quit`)
- Build Settings panel - packages the Sandbox runtime, compiled scripts, and shaders into a standalone output folder
- Material editing tools
- LookDev controls for lighting, environment, and MSAA
- Environment texture assignment tools
	- Diffuse environment map picker/clear
	- Specular environment map picker/clear
- Material texture assignment tools for albedo/normal/roughness/AO/emissive
- Visual Studio integration - new scripts open automatically alongside the scripting solution

### Tooling

- Shader compilation script for Vulkan shader pipeline
- `Sandbox` project - a minimal standalone runner (no editor UI) for running/shipping a scene as a real game

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
- .NET 8 SDK (required to build/run the C# gameplay scripts)
- Visual Studio (optional - used for the "open script" editor integration)
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

Gameplay scripts build as their own .NET projects and are picked up automatically by the editor
(including hot reload), but can also be built standalone from the command line:

- dotnet build Scripts/Piece.GameScripts

## Project Layout

- PieceLib/src/core: application, input, logging, background services
- PieceLib/src/renderer: Vulkan renderer, swapchain, passes, pipelines, systems
- PieceLib/src/scene: ECS scene, entities, components, world state
- PieceLib/src/assets: model importers and asset processing
- PieceLib/src/scripting: CoreCLR/hostfxr host and native/managed script bridge
- PieceEditor/src: editor UI and workflows
- PieceLib/src/shaders: shader sources and shader compile script
- Scripts/Piece.ScriptCore: stable C# engine bridge API (`ScriptBase`, `Entity`, `Transform`, ...)
- Scripts/Piece.GameScripts: user gameplay scripts, hot-reloadable at runtime
- Sandbox/: standalone shipped-game executable (no editor UI)

## Philosophy

Piece Engine aims to keep low-level graphics control without sacrificing structure.
The goal is practical Vulkan development with maintainable engine architecture.

## Contributing

Issues and pull requests are welcome.

## License

This project is licensed under the MIT License.
See [LICENSE](LICENSE).
