using System;
using System.Collections.Generic;
using System.Reflection;

namespace PieceEngine;

// Owns the live ScriptBase instances, keyed by the native entity UUID.
internal static class ScriptRegistry
{
    private static readonly Dictionary<long, ScriptBase> s_Instances = new();

    public static void Create(long entityId, string className)
    {
        Type? type = Assembly.GetExecutingAssembly().GetType(className);
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
