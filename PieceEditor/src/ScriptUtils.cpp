#include "ScriptUtils.h"

#include <core/Application.h>
#include <utils/platform/WindowsUtils.h>

#include <cctype>
#include <fstream>

namespace Piece::ScriptUtils {

namespace fs = std::filesystem;

std::string SanitizeClassName(const std::string& input) {
    std::string result;
    result.reserve(input.size());

    bool capitalizeNext = true;
    for (char c : input) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            result += capitalizeNext ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
            capitalizeNext = false;
        } else {
            capitalizeNext = true;
        }
    }

    if (result.empty()) {
        return "NewScript";
    }
    if (std::isdigit(static_cast<unsigned char>(result.front()))) {
        result.insert(result.begin(), '_');
    }
    return result;
}

std::filesystem::path FindScriptFile(const std::string& className) {
    if (className.empty()) {
        return {};
    }

    const size_t lastDot = className.find_last_of('.');
    const std::string simpleName = lastDot == std::string::npos ? className : className.substr(lastDot + 1);
    const std::string targetFileName = simpleName + ".cs";

    const fs::path searchRoots[] = {
        fs::path("Scripts") / "Piece.GameScripts" / "src",
        fs::path("Scripts") / "Piece.ScriptCore" / "src",
    };

    // Fast path: filename matches the class name (true for every engine-created script).
    for (const fs::path& root : searchRoots) {
        std::error_code error;
        if (!fs::exists(root, error)) {
            continue;
        }

        for (const auto& entry : fs::recursive_directory_iterator(root, error)) {
            if (error) {
                break;
            }
            if (entry.is_regular_file() && entry.path().filename() == targetFileName) {
                return entry.path();
            }
        }
    }

    // Slow path: filename differs from its class declaration (e.g. a manually renamed file) -
    // parse each .cs file's actual "class X" declaration and match against that instead.
    for (const fs::path& root : searchRoots) {
        std::error_code error;
        if (!fs::exists(root, error)) {
            continue;
        }

        for (const auto& entry : fs::recursive_directory_iterator(root, error)) {
            if (error) {
                break;
            }
            if (entry.is_regular_file() && entry.path().extension() == ".cs" && GetClassNameFromScriptFile(entry.path()) == simpleName) {
                return entry.path();
            }
        }
    }
    return {};
}

std::string GetClassNameFromScriptFile(const std::filesystem::path& scriptPath) {
    std::ifstream file(scriptPath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            const size_t classPos = line.find("class ");
            if (classPos == std::string::npos) {
                continue;
            }
            size_t nameStart = classPos + 6;
            while (nameStart < line.size() && std::isspace(static_cast<unsigned char>(line[nameStart]))) {
                ++nameStart;
            }
            size_t nameEnd = nameStart;
            while (nameEnd < line.size() && (std::isalnum(static_cast<unsigned char>(line[nameEnd])) || line[nameEnd] == '_')) {
                ++nameEnd;
            }
            if (nameEnd > nameStart) {
                return line.substr(nameStart, nameEnd - nameStart);
            }
        }
    }
    return scriptPath.stem().string();
}

std::string CreateScriptFile(const std::string& suggestedName, std::filesystem::path& outPath,
    const std::filesystem::path& targetDirectory) {
    const fs::path scriptsDir = targetDirectory.empty()
        ? fs::path("Scripts") / "Piece.GameScripts" / "src"
        : targetDirectory;
    std::error_code error;
    fs::create_directories(scriptsDir, error);

    const std::string baseName = SanitizeClassName(suggestedName);
    std::string className = baseName;
    fs::path scriptPath = scriptsDir / (className + ".cs");
    for (int suffix = 1; fs::exists(scriptPath); ++suffix) {
        className = baseName + std::to_string(suffix + 1);
        scriptPath = scriptsDir / (className + ".cs");
    }

    std::ofstream file(scriptPath);
    if (!file.is_open()) {
        return {};
    }

    file << "using PieceEngine;\n\n"
         << "public class " << className << " : ScriptBase\n"
         << "{\n"
         << "    public override void OnCreate()\n"
         << "    {\n\n"
         << "    }\n\n"
         << "    public override void OnUpdate(float deltaTime)\n"
         << "    {\n\n"
         << "    }\n"
         << "}\n";
    file.close();

    outPath = scriptPath;
    return className;
}

bool OpenScriptInVisualStudio(const std::filesystem::path& scriptPath) {
    const fs::path solutionPath = fs::path("Scripts") / "Piece.ScriptCore" / "Piece.ScriptCore.slnx";
    return Platform::OpenFileInVisualStudio(solutionPath.string(), scriptPath.string());
}

namespace {

std::string GameScriptsDllPath() {
    return (fs::path("Scripts") / "Piece.GameScripts" / "bin" / "Piece.GameScripts.dll").string();
}

// Skips the (slow, ~1s) dotnet build invocation when nothing under GameScripts/src has changed
// since the last successful build - this is the common case (e.g. pressing Play repeatedly
// without touching any script). Only a real source edit should pay the recompile cost.
bool GameScriptsOutOfDate() {
    const fs::path dllPath = GameScriptsDllPath();
    std::error_code error;
    if (!fs::exists(dllPath, error) || error) {
        return true;
    }
    const auto dllTime = fs::last_write_time(dllPath, error);
    if (error) {
        return true;
    }

    const fs::path srcDir = fs::path("Scripts") / "Piece.GameScripts" / "src";
    for (const auto& entry : fs::recursive_directory_iterator(srcDir, error)) {
        if (error) {
            return true;
        }
        if (!entry.is_regular_file() || entry.path().extension() != ".cs") {
            continue;
        }
        std::error_code entryError;
        const auto srcTime = fs::last_write_time(entry.path(), entryError);
        if (entryError || srcTime > dllTime) {
            return true;
        }
    }
    return false;
}

} // namespace

void RecompileAndReloadGameScripts(std::function<void(bool, std::string)> onComplete) {
    if (!GameScriptsOutOfDate()) {
        onComplete(true, "Scripts already up to date.");
        return;
    }

    // --no-dependencies: Piece.ScriptCore is loaded directly into this process via hostfxr (not via
    // a collectible ALC/stream), so its bin/Piece.ScriptCore.dll is locked for the lifetime of the
    // editor process. Without this flag, MSBuild tries to rebuild+overwrite it as a project reference
    // and fails/retries on the file lock. GameScripts only needs ScriptCore's compile-time reference
    // assembly (already on disk from the last manual ScriptCore build), not a fresh rebuild of it.
    Platform::RunProcessAsync("dotnet build Scripts/Piece.GameScripts --no-dependencies", "",
        [onComplete = std::move(onComplete)](int exitCode, std::string output) {
            if (exitCode != 0) {
                onComplete(false, "Script compilation failed:\n" + output);
                return;
            }

            Piece::Application::Get().GetScriptEngine().LoadGameScripts(GameScriptsDllPath());
            onComplete(true, "Scripts compiled and reloaded.");
        });
}

bool RecompileAndReloadGameScriptsBlocking(std::string& outMessage) {
    if (!GameScriptsOutOfDate()) {
        outMessage = "Scripts already up to date.";
        return true;
    }

    int exitCode = -1;
    std::string output;
    // See RecompileAndReloadGameScripts for why --no-dependencies is required here.
    Platform::RunProcess("dotnet build Scripts/Piece.GameScripts --no-dependencies", "", exitCode, output);

    if (exitCode != 0) {
        outMessage = "Script compilation failed:\n" + output;
        return false;
    }

    Piece::Application::Get().GetScriptEngine().LoadGameScripts(GameScriptsDllPath());
    outMessage = "Scripts compiled and reloaded.";
    return true;
}

} // namespace Piece::ScriptUtils
