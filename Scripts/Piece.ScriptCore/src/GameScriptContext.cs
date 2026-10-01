using System;
using System.IO;
using System.Reflection;
using System.Runtime.Loader;

namespace PieceEngine;

// Collectible load context so game scripts can be recompiled and reloaded without restarting the host.
internal sealed class GameScriptContext : AssemblyLoadContext
{
    public Assembly GameAssembly { get; }

    public GameScriptContext(string assemblyPath) : base(isCollectible: true)
    {
        // Loading from a stream (not LoadFromAssemblyPath) avoids locking the file on disk,
        // so the next `dotnet build` can overwrite it while this context is still alive/unloading.
        using FileStream stream = File.OpenRead(assemblyPath);
        GameAssembly = LoadFromStream(stream);
    }

    protected override Assembly? Load(AssemblyName assemblyName)
    {
        // Resolve dependencies (Piece.ScriptCore, BCL, etc.) from whatever is already loaded elsewhere
        // in the process instead of duplicating them into this collectible context. Returning null
        // does NOT fall back to AssemblyLoadContext.Default here: Piece.ScriptCore is loaded via
        // CoreCLR's component-hosting API (load_assembly_and_get_function_pointer), which places it in
        // its own "IsolatedComponentLoadContext", NOT AssemblyLoadContext.Default. So resolution must
        // search every loaded ALC (AssemblyLoadContext.All), not just Default.
        foreach (AssemblyLoadContext alc in AssemblyLoadContext.All) {
            foreach (Assembly assembly in alc.Assemblies) {
                if (string.Equals(assembly.GetName().Name, assemblyName.Name, StringComparison.OrdinalIgnoreCase)) {
                    return assembly;
                }
            }
        }
        return null;
    }
}
