//using System.Collections;
//using System.Collections.Generic;
//using UnityEngine;
//using TMPro;
//using System;
//using System.Runtime.InteropServices;

//public class ResearchModeScript : MonoBehaviour
//{
//    // Start is called before the first frame update

//    private int Width;
//    private int Height;
//    private int Length;
//    public TMP_Text InfoTex;
//    private string resolution;
//    private string firstpixels;


//    void Start()
//    {

//#if !UNITY_EDITOR && UNITY_METRO

//        int a = 0;
        
//        RM.initRSC();
//        InfoTex.text = "started:" + a;


//        //texture2D = new Texture2D(Width, Height);

//        //meshRenderer = GameObject.Find("Preview Plane2").GetComponent<MeshRenderer>();

//        //RM.ReleaseResMemory(returnedPtr);

//        //StartCoroutine(Routine());
//#endif
//    }





//    // Update is called once per frame
//    void Update()
//    {

//#if !UNITY_EDITOR && UNITY_METRO

//        //InfoTex.text = "update";


//        //int a= RM.Resolution();


//        InfoTex.text = "W:" ;



//        //IntPtr returnedPtr = RM.Resolution();

//        //int[] Res = new int[3];

//        //Marshal.Copy(returnedPtr, Res, 0, 3);

//        //Width = Res[0];
//        //Height = Res[1];
//        //Length = Res[2];

//        //resolution = Width + "x" + Height + "/" + Length;
//        //InfoTex.text = "res:" + resolution + "\n" + "pix:" + firstpixels;

//        ////int a = RM.plus(2, 3);
//        ////InfoTex.text = "update:" + a;

//#endif

//    }
//}
