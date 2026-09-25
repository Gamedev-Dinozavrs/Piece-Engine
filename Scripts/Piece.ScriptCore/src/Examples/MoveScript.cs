using System.Numerics;

namespace PieceEngine.Examples;

// Attach via ScriptComponent.className = "PieceEngine.Examples.MoveScript". WASD moves on the XZ plane.
public class MoveScript : ScriptBase
{
    private const float Speed = 3.0f;

    public override void OnUpdate(float deltaTime)
    {
        Vector3 direction = Vector3.Zero;
        if (IsKeyPressed(KeyCode.W)) direction.Z -= 1.0f;
        if (IsKeyPressed(KeyCode.S)) direction.Z += 1.0f;
        if (IsKeyPressed(KeyCode.A)) direction.X -= 1.0f;
        if (IsKeyPressed(KeyCode.D)) direction.X += 1.0f;

        if (direction != Vector3.Zero) {
            Transform.Position += Vector3.Normalize(direction) * Speed * deltaTime;
        }
    }
}
