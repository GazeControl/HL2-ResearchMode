using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using RosSharp.RosBridgeClient;

public class MarkControl : MonoBehaviour
{
    // Start is called before the first frame update
    public Transform MirroControlObject;
    public Transform RealMarkobject;
    public ControlCommandPublisher Publisher;

    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        
    }

    public void StartPublish()
    {
        Debug.Log("Start Publish control Command!!!");

        MirroControlObject.localPosition = transform.localPosition;
        MirroControlObject.localRotation = transform.localRotation;

        var relativePosition = RealMarkobject.InverseTransformPoint(transform.position);
        Quaternion relativeRotation = Quaternion.Inverse(RealMarkobject.rotation) * transform.rotation;


        Debug.Log(relativePosition);
        Publisher.UpdateMessage(relativePosition, relativeRotation);


    }
}
