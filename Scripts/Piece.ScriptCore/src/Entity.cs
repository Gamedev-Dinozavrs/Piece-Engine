namespace PieceEngine;

public sealed class Entity
{
    public long Id { get; }
    public Transform Transform { get; }

    internal Entity(long id)
    {
        Id = id;
        Transform = new Transform(id);
    }
}
