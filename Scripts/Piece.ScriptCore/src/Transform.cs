using System.Numerics;

namespace PieceEngine;

public sealed class Transform
{
    private readonly long m_EntityId;

    internal Transform(long entityId)
    {
        m_EntityId = entityId;
    }

    public Vector3 Position
    {
        get => EngineBridge.GetPosition(m_EntityId);
        set => EngineBridge.SetPosition(m_EntityId, value);
    }

    // Degrees, matching the editor's Transform panel.
    public Vector3 Rotation
    {
        get => EngineBridge.GetRotation(m_EntityId);
        set => EngineBridge.SetRotation(m_EntityId, value);
    }

    public Vector3 Scale
    {
        get => EngineBridge.GetScale(m_EntityId);
        set => EngineBridge.SetScale(m_EntityId, value);
    }
}
