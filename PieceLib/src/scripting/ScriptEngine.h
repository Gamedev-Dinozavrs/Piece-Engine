#pragma once

#include <cstdint>
#include <string>

namespace Piece {

// Minimal native host for the managed (C#) scripting runtime.
// Boots CoreCLR via hostfxr and resolves a handful of UnmanagedCallersOnly
// entry points exposed by PieceEngine.NativeBridge.
class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();

    // managedAssemblyDir must contain Piece.ScriptCore.dll and Piece.ScriptCore.runtimeconfig.json.
    bool Initialize(const std::string& managedAssemblyDir);
    void Shutdown();

    bool IsInitialized() const;

    // Calls PieceEngine.NativeBridge.Ping() to prove the managed bridge is alive.
    bool Ping();

    // className must be a namespace-qualified type name (within the Piece.ScriptCore assembly)
    // deriving from ScriptBase, e.g. "PieceEngine.Examples.LogScript".
    void CreateEntityScript(uint64_t entityId, const std::string& className);
    void UpdateEntityScript(uint64_t entityId, float deltaTime);
    void DestroyEntityScript(uint64_t entityId);

private:
    bool LoadHostFxr();
    bool LoadRuntimeDelegate(const std::string& runtimeConfigPath);
    void* GetManagedFunctionPointer(const std::string& typeName, const std::string& methodName);
    bool RegisterEngineCallbacks();

    void* m_HostfxrModule = nullptr;
    void* m_InitializeForRuntimeConfig = nullptr; // hostfxr_initialize_for_runtime_config_fn
    void* m_GetRuntimeDelegate = nullptr;          // hostfxr_get_runtime_delegate_fn
    void* m_CloseHostContext = nullptr;            // hostfxr_close_fn
    void* m_HostContextHandle = nullptr;           // hostfxr_handle
    void* m_LoadAssemblyAndGetFunctionPointer = nullptr; // load_assembly_and_get_function_pointer_fn

    void* m_PingFn = nullptr;
    void* m_CreateEntityScriptFn = nullptr;
    void* m_UpdateEntityScriptFn = nullptr;
    void* m_DestroyEntityScriptFn = nullptr;

    bool m_Initialized = false;
    std::string m_ManagedAssemblyDir;
    std::string m_AssemblyPath;
};

} // namespace Piece

