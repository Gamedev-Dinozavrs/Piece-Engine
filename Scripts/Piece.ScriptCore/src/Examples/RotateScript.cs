using System.Numerics;

namespace PieceEngine.Examples;

// Attach via ScriptComponent.className = "PieceEngine.Examples.RotateScript".
public class RotateScript : ScriptBase
{
    private const float DegreesPerSecond = 45.0f;

    public override void OnUpdate(float deltaTime)
    {
        Vector3 rotation = Transform.Rotation;
        rotation.Y += DegreesPerSecond * deltaTime;
        Transform.Rotation = rotation;
    }
}
