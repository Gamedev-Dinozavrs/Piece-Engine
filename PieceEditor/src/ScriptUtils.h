#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace Piece::ScriptUtils {

// Sanitizes arbitrary text (e.g. an entity's tag) into a valid C# class identifier.
std::string SanitizeClassName(const std::string& input);

// Searches Scripts/Piece.ScriptCore/src and Scripts/Piece.GameScripts/src (recursively) for a .cs
// file matching the class name's simple (last) segment, e.g.
// "PieceEngine.Examples.RotateScript" -> "RotateScript.cs". Returns an empty path if not found.
std::filesystem::path FindScriptFile(const std::string& className);

// Parses the given .cs file for its "class X : ScriptBase" declaration (falls back to the file's
// stem if no match is found). Used to resolve a dropped script asset's class name.
std::string GetClassNameFromScriptFile(const std::filesystem::path& scriptPath);

// Creates a new ScriptBase-derived .cs file, auto-numbering the class name if it already exists
// (e.g. "Cube" -> "Cube2"). Writes into targetDirectory, or Scripts/Piece.GameScripts/src if
// targetDirectory is empty. Returns the final class name used, or an empty string on failure.
// outPath receives the created file's path.
std::string CreateScriptFile(const std::string& suggestedName, std::filesystem::path& outPath,
    const std::filesystem::path& targetDirectory = {});

// Opens the given script file in Visual Studio alongside the Piece.ScriptCore solution.
bool OpenScriptInVisualStudio(const std::filesystem::path& scriptPath);

// Rebuilds Scripts/Piece.GameScripts (dotnet build) and, on success, hot-reloads the resulting
// assembly into the running script host via ScriptEngine::LoadGameScripts - no process restart
// needed. onComplete(success, message) is invoked on the main thread.
void RecompileAndReloadGameScripts(std::function<void(bool, std::string)> onComplete);

// Blocking variant of RecompileAndReloadGameScripts. Returns true on success. Used right before
// entering Play mode so the scene always runs against freshly compiled script code.
bool RecompileAndReloadGameScriptsBlocking(std::string& outMessage);

} // namespace Piece::ScriptUtils
