using System;
using System.Collections.Generic;
using System.Reflection;

namespace PieceEngine;

// Owns the live ScriptBase instances, keyed by the native entity UUID.
internal static class ScriptRegistry
{
    private static readonly Dictionary<long, ScriptBase> s_Instances = new();
    private static GameScriptContext? s_GameContext;

    // Loads (or reloads) the user game-scripts assembly into a fresh collectible context, so newly
    // compiled script code takes effect without restarting the host process.
    public static void LoadGameScripts(string assemblyPath)
    {
        foreach (ScriptBase instance in s_Instances.Values) {
            instance.OnDestroy();
        }
        s_Instances.Clear();

        GameScriptContext? previous = s_GameContext;
        s_GameContext = new GameScriptContext(assemblyPath);
        previous?.Unload();
    }

    private static Type? ResolveType(string className)
    {
        return s_GameContext?.GameAssembly.GetType(className)
            ?? Assembly.GetExecutingAssembly().GetType(className);
    }

    public static void Create(long entityId, string className)
    {
        Type? type = ResolveType(className);
        if (type == null || !typeof(ScriptBase).IsAssignableFrom(type)) {
            Console.WriteLine($"[Piece.ScriptCore] Script type '{className}' not found or does not derive from ScriptBase.");
            return;
        }

        if (Activator.CreateInstance(type) is not ScriptBase instance) {
            return;
        }

        instance.EntityId = entityId;
        s_Instances[entityId] = instance;
        instance.OnCreate();
    }

    public static void Update(long entityId, float deltaTime)
    {
        if (s_Instances.TryGetValue(entityId, out ScriptBase? instance)) {
            instance.OnUpdate(deltaTime);
        }
    }

    public static void Destroy(long entityId)
    {
        if (s_Instances.Remove(entityId, out ScriptBase? instance)) {
            instance.OnDestroy();
        }
    }
}
