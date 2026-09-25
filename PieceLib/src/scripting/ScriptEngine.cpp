#include <PiecePCH.h>

#include "ScriptEngine.h"

#include "ScriptGlue.h"
#include "hostfxr/coreclr_delegates.h"
#include "hostfxr/hostfxr.h"

#include <core/Log.h>

#include <cstdlib>
#include <filesystem>

namespace Piece {

namespace {

namespace fs = std::filesystem;

std::wstring ToWideString(const std::string& value) {
    return fs::path(value).wstring();
}

std::string DotnetRootDirectory() {
    if (const char* envRoot = std::getenv("DOTNET_ROOT")) {
        return envRoot;
    }

    if (const char* programFiles = std::getenv("ProgramFiles")) {
        return std::string(programFiles) + "\\dotnet";
    }

    return "C:\\Program Files\\dotnet";
}

// Parses folder names like "10.0.11" so the highest installed hostfxr can be picked.
bool ParseVersion(const std::string& text, std::vector<int>& outParts) {
    outParts.clear();
    std::stringstream stream(text);
    std::string part;
    while (std::getline(stream, part, '.')) {
        try {
            outParts.push_back(std::stoi(part));
        } catch (...) {
            return false;
        }
    }
    return !outParts.empty();
}

bool IsVersionLess(const std::vector<int>& a, const std::vector<int>& b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

std::wstring FindHostFxrPath() {
    const fs::path fxrRoot = fs::path(DotnetRootDirectory()) / "host" / "fxr";

    std::error_code errorCode;
    if (!fs::exists(fxrRoot, errorCode)) {
        PIECE_CORE_ERROR("hostfxr directory not found: {}", fxrRoot.string());
        return {};
    }

    fs::path bestPath;
    std::vector<int> bestVersion;

    for (const auto& entry : fs::directory_iterator(fxrRoot, errorCode)) {
        if (!entry.is_directory()) {
            continue;
        }

        std::vector<int> version;
        if (!ParseVersion(entry.path().filename().string(), version)) {
            continue;
        }

        if (bestPath.empty() || IsVersionLess(bestVersion, version)) {
            bestVersion = version;
            bestPath = entry.path();
        }
    }

    if (bestPath.empty()) {
        PIECE_CORE_ERROR("No hostfxr version folders found under {}", fxrRoot.string());
        return {};
    }

    return (bestPath / "hostfxr.dll").wstring();
}

} // namespace

ScriptEngine::ScriptEngine() = default;
ScriptEngine::~ScriptEngine() {
    Shutdown();
}

bool ScriptEngine::LoadHostFxr() {
    const std::wstring hostfxrPath = FindHostFxrPath();
    if (hostfxrPath.empty()) {
        return false;
    }

    HMODULE module = ::LoadLibraryW(hostfxrPath.c_str());
    if (!module) {
        PIECE_CORE_ERROR("Failed to load hostfxr.dll");
        return false;
    }

    m_HostfxrModule = module;
    m_InitializeForRuntimeConfig = reinterpret_cast<void*>(
        ::GetProcAddress(module, "hostfxr_initialize_for_runtime_config"));
    m_GetRuntimeDelegate = reinterpret_cast<void*>(
        ::GetProcAddress(module, "hostfxr_get_runtime_delegate"));
    m_CloseHostContext = reinterpret_cast<void*>(
        ::GetProcAddress(module, "hostfxr_close"));

    if (!m_InitializeForRuntimeConfig || !m_GetRuntimeDelegate || !m_CloseHostContext) {
        PIECE_CORE_ERROR("Failed to resolve hostfxr exports");
        return false;
    }

    return true;
}

bool ScriptEngine::LoadRuntimeDelegate(const std::string& runtimeConfigPath) {
    const std::wstring wideConfigPath = ToWideString(runtimeConfigPath);

    auto initializeForRuntimeConfig =
        reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(m_InitializeForRuntimeConfig);
    auto getRuntimeDelegate =
        reinterpret_cast<hostfxr_get_runtime_delegate_fn>(m_GetRuntimeDelegate);

    hostfxr_handle contextHandle = nullptr;
    int32_t result = initializeForRuntimeConfig(wideConfigPath.c_str(), nullptr, &contextHandle);
    if (result != 0 || !contextHandle) {
        PIECE_CORE_ERROR("hostfxr_initialize_for_runtime_config failed with code {} for '{}'", result, runtimeConfigPath);
        return false;
    }

    m_HostContextHandle = contextHandle;

    void* delegatePtr = nullptr;
    result = getRuntimeDelegate(contextHandle, hdt_load_assembly_and_get_function_pointer, &delegatePtr);
    if (result != 0 || !delegatePtr) {
        PIECE_CORE_ERROR("hostfxr_get_runtime_delegate failed with code {}", result);
        return false;
    }

    m_LoadAssemblyAndGetFunctionPointer = delegatePtr;
    return true;
}

void* ScriptEngine::GetManagedFunctionPointer(const std::string& typeName, const std::string& methodName) {
    if (!m_LoadAssemblyAndGetFunctionPointer) {
        return nullptr;
    }

    auto loadAssemblyAndGetFunctionPointer =
        reinterpret_cast<load_assembly_and_get_function_pointer_fn>(m_LoadAssemblyAndGetFunctionPointer);

    const std::wstring wideAssemblyPath = ToWideString(m_AssemblyPath);
    const std::wstring wideTypeName = ToWideString(typeName);
    const std::wstring wideMethodName = ToWideString(methodName);

    void* functionPointer = nullptr;
    const int32_t result = loadAssemblyAndGetFunctionPointer(
        wideAssemblyPath.c_str(),
        wideTypeName.c_str(),
        wideMethodName.c_str(),
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        &functionPointer);

    if (result != 0 || !functionPointer) {
        PIECE_CORE_ERROR("Failed to resolve managed method '{}::{}' (code {})", typeName, methodName, result);
        return nullptr;
    }

    return functionPointer;
}

bool ScriptEngine::RegisterEngineCallbacks() {
    const std::string bridgeType = "PieceEngine.NativeBridge, Piece.ScriptCore";
    void* registerFn = GetManagedFunctionPointer(bridgeType, "RegisterEngineCallbacks");
    if (!registerFn) {
        return false;
    }

    using RegisterEngineCallbacksFn = void(CORECLR_DELEGATE_CALLTYPE*)(
        void*, void*, void*, void*, void*, void*, void*);

    reinterpret_cast<RegisterEngineCallbacksFn>(registerFn)(
        reinterpret_cast<void*>(&ScriptGlue::GetPosition),
        reinterpret_cast<void*>(&ScriptGlue::SetPosition),
        reinterpret_cast<void*>(&ScriptGlue::GetRotation),
        reinterpret_cast<void*>(&ScriptGlue::SetRotation),
        reinterpret_cast<void*>(&ScriptGlue::GetScale),
        reinterpret_cast<void*>(&ScriptGlue::SetScale),
        reinterpret_cast<void*>(&ScriptGlue::IsKeyPressed));

    return true;
}

bool ScriptEngine::Initialize(const std::string& managedAssemblyDir) {
    if (m_Initialized) {
        return true;
    }

    m_ManagedAssemblyDir = managedAssemblyDir;
    m_AssemblyPath = (fs::path(managedAssemblyDir) / "Piece.ScriptCore.dll").string();
    const std::string runtimeConfigPath = (fs::path(managedAssemblyDir) / "Piece.ScriptCore.runtimeconfig.json").string();

    if (!fs::exists(m_AssemblyPath) || !fs::exists(runtimeConfigPath)) {
        PIECE_CORE_ERROR("Managed script assembly or runtimeconfig not found in '{}'. Build Scripts/Piece.ScriptCore first.", managedAssemblyDir);
        return false;
    }

    if (!LoadHostFxr()) {
        return false;
    }

    if (!LoadRuntimeDelegate(runtimeConfigPath)) {
        return false;
    }

    const std::string bridgeType = "PieceEngine.NativeBridge, Piece.ScriptCore";
    m_PingFn = GetManagedFunctionPointer(bridgeType, "Ping");
    m_CreateEntityScriptFn = GetManagedFunctionPointer(bridgeType, "CreateEntityScript");
    m_UpdateEntityScriptFn = GetManagedFunctionPointer(bridgeType, "UpdateEntityScript");
    m_DestroyEntityScriptFn = GetManagedFunctionPointer(bridgeType, "DestroyEntityScript");

    if (!m_PingFn || !m_CreateEntityScriptFn || !m_UpdateEntityScriptFn || !m_DestroyEntityScriptFn) {
        PIECE_CORE_ERROR("Failed to resolve one or more NativeBridge methods.");
        return false;
    }

    if (!RegisterEngineCallbacks()) {
        return false;
    }

    m_Initialized = true;
    PIECE_CORE_INFO("Script runtime initialized from '{}'.", managedAssemblyDir);
    return true;
}

void ScriptEngine::Shutdown() {
    if (m_HostContextHandle && m_CloseHostContext) {
        reinterpret_cast<hostfxr_close_fn>(m_CloseHostContext)(m_HostContextHandle);
        m_HostContextHandle = nullptr;
    }

    if (m_HostfxrModule) {
        ::FreeLibrary(static_cast<HMODULE>(m_HostfxrModule));
        m_HostfxrModule = nullptr;
    }

    m_InitializeForRuntimeConfig = nullptr;
    m_GetRuntimeDelegate = nullptr;
    m_CloseHostContext = nullptr;
    m_LoadAssemblyAndGetFunctionPointer = nullptr;
    m_PingFn = nullptr;
    m_CreateEntityScriptFn = nullptr;
    m_UpdateEntityScriptFn = nullptr;
    m_DestroyEntityScriptFn = nullptr;
    m_Initialized = false;
}

bool ScriptEngine::IsInitialized() const {
    return m_Initialized;
}

bool ScriptEngine::Ping() {
    if (!m_Initialized || !m_PingFn) {
        return false;
    }

    reinterpret_cast<void(CORECLR_DELEGATE_CALLTYPE*)()>(m_PingFn)();
    return true;
}

void ScriptEngine::CreateEntityScript(uint64_t entityId, const std::string& className) {
    if (!m_Initialized || !m_CreateEntityScriptFn) {
        return;
    }

    const std::wstring wideClassName = ToWideString(className);
    reinterpret_cast<void(CORECLR_DELEGATE_CALLTYPE*)(int64_t, const wchar_t*)>(m_CreateEntityScriptFn)(
        static_cast<int64_t>(entityId), wideClassName.c_str());
}

void ScriptEngine::UpdateEntityScript(uint64_t entityId, float deltaTime) {
    if (!m_Initialized || !m_UpdateEntityScriptFn) {
        return;
    }

    reinterpret_cast<void(CORECLR_DELEGATE_CALLTYPE*)(int64_t, float)>(m_UpdateEntityScriptFn)(static_cast<int64_t>(entityId), deltaTime);
}

void ScriptEngine::DestroyEntityScript(uint64_t entityId) {
    if (!m_Initialized || !m_DestroyEntityScriptFn) {
        return;
    }

    reinterpret_cast<void(CORECLR_DELEGATE_CALLTYPE*)(int64_t)>(m_DestroyEntityScriptFn)(static_cast<int64_t>(entityId));
}

} // namespace Piece

