using System;
using System.Runtime.InteropServices;

namespace PieceEngine;

// Entry points resolved by the native ScriptEngine via load_assembly_and_get_function_pointer.
// Each method must stay a public static method marked [UnmanagedCallersOnly] with a blittable signature.
// Every method body is wrapped in try/catch: an unhandled exception crossing this boundary
// fail-fasts the whole native process with no diagnostics, so we must catch and log here.
public static class NativeBridge
{
    [UnmanagedCallersOnly]
    public static void Ping()
    {
        Console.WriteLine("[Piece.ScriptCore] Native bridge is alive.");
    }

    [UnmanagedCallersOnly]
    public static void CreateEntityScript(long entityId, IntPtr classNamePtr)
    {
        try {
            string? className = Marshal.PtrToStringUni(classNamePtr);
            if (string.IsNullOrEmpty(className)) {
                return;
            }

            ScriptRegistry.Create(entityId, className);
        } catch (Exception exception) {
            Console.Error.WriteLine($"[Piece.ScriptCore] CreateEntityScript failed: {exception}");
        }
    }

    [UnmanagedCallersOnly]
    public static void UpdateEntityScript(long entityId, float deltaTime)
    {
        try {
            ScriptRegistry.Update(entityId, deltaTime);
        } catch (Exception exception) {
            Console.Error.WriteLine($"[Piece.ScriptCore] UpdateEntityScript failed: {exception}");
        }
    }

    [UnmanagedCallersOnly]
    public static void DestroyEntityScript(long entityId)
    {
        try {
            ScriptRegistry.Destroy(entityId);
        } catch (Exception exception) {
            Console.Error.WriteLine($"[Piece.ScriptCore] DestroyEntityScript failed: {exception}");
        }
    }

    [UnmanagedCallersOnly]
    public static unsafe void RegisterEngineCallbacks(
        delegate* unmanaged<long, float*, float*, float*, void> getPosition,
        delegate* unmanaged<long, float, float, float, void> setPosition,
        delegate* unmanaged<long, float*, float*, float*, void> getRotation,
        delegate* unmanaged<long, float, float, float, void> setRotation,
        delegate* unmanaged<long, float*, float*, float*, void> getScale,
        delegate* unmanaged<long, float, float, float, void> setScale,
        delegate* unmanaged<int, int> isKeyPressed)
    {
        try {
            EngineBridge.Register(getPosition, setPosition, getRotation, setRotation, getScale, setScale, isKeyPressed);
        } catch (Exception exception) {
            Console.Error.WriteLine($"[Piece.ScriptCore] RegisterEngineCallbacks failed: {exception}");
        }
    }
}
