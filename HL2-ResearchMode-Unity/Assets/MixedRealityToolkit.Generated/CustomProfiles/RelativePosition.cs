using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class RelativePosition : MonoBehaviour
{
    // Start is called before the first frame update

    public Transform Camera;
    public Transform World;
    public Transform CameraFrame;
    public Transform Render;
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        var relativePosition = World.InverseTransformPoint(Camera.position);
        Quaternion relativeRotation = Quaternion.Inverse(World.rotation) * Camera.rotation;
        CameraFrame.localPosition = relativePosition;
        CameraFrame.localRotation = relativeRotation;
        Render.localPosition = -World.position;
        Render.localEulerAngles= new Vector3(0, -World.eulerAngles.y, 0);
    }
}
