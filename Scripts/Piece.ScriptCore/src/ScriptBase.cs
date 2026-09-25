namespace PieceEngine;

public abstract class ScriptBase
{
    public long EntityId { get; internal set; }

    private Entity? m_Entity;
    public Entity Entity => m_Entity ??= new Entity(EntityId);
    public Transform Transform => Entity.Transform;

    protected static bool IsKeyPressed(KeyCode key) => EngineBridge.IsKeyPressed((int)key);

    public virtual void OnCreate() { }
    public virtual void OnUpdate(float deltaTime) { }
    public virtual void OnDestroy() { }
}
