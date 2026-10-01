using PieceEngine;

public class NewScript : ScriptBase
{
    public override void OnCreate()
    {

    }

    public override void OnUpdate(float deltaTime)
    {
        Transform.Position += new System.Numerics.Vector3(1, 0, 0) * deltaTime;
    }
}
