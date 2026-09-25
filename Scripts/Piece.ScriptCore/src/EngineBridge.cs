using System.Numerics;

namespace PieceEngine;

// Wraps native function pointers registered once via NativeBridge.RegisterEngineCallbacks.
// Uses raw unmanaged function pointers (not Marshal.GetDelegateForFunctionPointer) to avoid
// the extra marshaling-stub indirection when calling straight into native free functions.
internal static unsafe class EngineBridge
{
    private static delegate* unmanaged<long, float*, float*, float*, void> s_GetPosition;
    private static delegate* unmanaged<long, float, float, float, void> s_SetPosition;
    private static delegate* unmanaged<long, float*, float*, float*, void> s_GetRotation;
    private static delegate* unmanaged<long, float, float, float, void> s_SetRotation;
    private static delegate* unmanaged<long, float*, float*, float*, void> s_GetScale;
    private static delegate* unmanaged<long, float, float, float, void> s_SetScale;
    private static delegate* unmanaged<int, int> s_IsKeyPressed;

    public static void Register(
        delegate* unmanaged<long, float*, float*, float*, void> getPosition,
        delegate* unmanaged<long, float, float, float, void> setPosition,
        delegate* unmanaged<long, float*, float*, float*, void> getRotation,
        delegate* unmanaged<long, float, float, float, void> setRotation,
        delegate* unmanaged<long, float*, float*, float*, void> getScale,
        delegate* unmanaged<long, float, float, float, void> setScale,
        delegate* unmanaged<int, int> isKeyPressed)
    {
        s_GetPosition = getPosition;
        s_SetPosition = setPosition;
        s_GetRotation = getRotation;
        s_SetRotation = setRotation;
        s_GetScale = getScale;
        s_SetScale = setScale;
        s_IsKeyPressed = isKeyPressed;
    }

    public static Vector3 GetPosition(long entityId)
    {
        float x, y, z;
        s_GetPosition(entityId, &x, &y, &z);
        return new Vector3(x, y, z);
    }

    public static void SetPosition(long entityId, Vector3 value) => s_SetPosition(entityId, value.X, value.Y, value.Z);

    public static Vector3 GetRotation(long entityId)
    {
        float x, y, z;
        s_GetRotation(entityId, &x, &y, &z);
        return new Vector3(x, y, z);
    }

    public static void SetRotation(long entityId, Vector3 value) => s_SetRotation(entityId, value.X, value.Y, value.Z);

    public static Vector3 GetScale(long entityId)
    {
        float x, y, z;
        s_GetScale(entityId, &x, &y, &z);
        return new Vector3(x, y, z);
    }

    public static void SetScale(long entityId, Vector3 value) => s_SetScale(entityId, value.X, value.Y, value.Z);

    public static bool IsKeyPressed(int keyCode) => s_IsKeyPressed(keyCode) != 0;
}
