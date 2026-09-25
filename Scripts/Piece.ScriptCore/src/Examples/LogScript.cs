namespace PieceEngine.Examples;

// Minimal proof-of-life script: attach via ScriptComponent.className = "PieceEngine.Examples.LogScript".
public class LogScript : ScriptBase
{
    public override void OnCreate()
    {
        Console.WriteLine($"[LogScript] Created on entity {EntityId}");
    }

    public override void OnUpdate(float deltaTime)
    {
        Console.WriteLine($"[LogScript] Update entity {EntityId} dt={deltaTime}");
    }

    public override void OnDestroy()
    {
        Console.WriteLine($"[LogScript] Destroyed on entity {EntityId}");
    }
}
