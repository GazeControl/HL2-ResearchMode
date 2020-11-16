using UnityEngine;
using System;
using System.Runtime.InteropServices;
using UnityEngine.UI;
using TMPro;
//using System.Diagnostics;
using RosSharp.RosBridgeClient;

#if ENABLE_WINMD_SUPPORT
using HL2UnityPlugin;
#endif

public class RMPlugin : MonoBehaviour
{
#if ENABLE_WINMD_SUPPORT
    HL2ResearchMode researchMode;
#endif

    //[SerializeField]

    //[SerializeField]
    public Text textOut;
    private Material mediaMaterial = null;
    private Material mediaMaterial2 = null;
    private Material mediaMaterial3 = null;
    private Material mediaMaterial4 = null;
    private Texture2D mediaTexture = null;
    private Texture2D mediaTexture2 = null;
    private Texture2D mediaTexture3 = null;
    private Texture2D mediaTexture4 = null;
    //int Width = 512;
    //int Height = 512;
    int Width = 320;
    int Height = 288;

    public static byte[] GrayScale1frameData = null;
    public static byte[] GrayScale2frameData = null;
    public static byte[] Depth1frameData = null;
    public static byte[] Depth2frameData = null;

    public static bool GrayScale1Updated = false;
    public static bool GrayScale2Updated = false;
    public static bool Depth1Updated = false;
    public static bool Depth2Updated = false;
    public static bool IMUUpdated = false;

    private bool GrayScale1Enabled = true;
    private bool GrayScale2Enabled = true;
    private bool DepthEnabled = true;
    private bool Depth2Enabled = true;
    private bool IMUEnabled = true;

    public static float[] Acc ;
    public static float[] Gyro ;
    public static float[] Mag;

    //public Button IMU;
    //public Button GrayL;
    //public Button GrayR;
    //public Button Depth;

    private bool textureInitialized;
    public bool preview;

    public GameObject previewPlane1 = null;
    public GameObject previewPlane2 = null;
    public GameObject previewPlane3 = null;
    public GameObject previewPlane4 = null;

    void Start()
    {
        //IMU.onClick.AddListener(IMUButtonTask);
        //GrayL.onClick.AddListener(GrayLButtonTask);
        //GrayR.onClick.AddListener(GrayRButtonTask);
        //Depth.onClick.AddListener(DepthButtonTask);


#if ENABLE_WINMD_SUPPORT
        researchMode = new HL2ResearchMode();
        researchMode.InitializeDepthSensor();
        //researchMode.SetReferenceCoordinateSystem(unityWorldOrigin);
#endif
        mediaMaterial = previewPlane1.GetComponent<MeshRenderer>().material;
        mediaMaterial2 = previewPlane2.GetComponent<MeshRenderer>().material;
        mediaMaterial3 = previewPlane3.GetComponent<MeshRenderer>().material;
        mediaMaterial4 = previewPlane4.GetComponent<MeshRenderer>().material;
#if ENABLE_WINMD_SUPPORT
        StartDepthSensingLoopEvent();
#endif
        //InvokeRepeating("Process", 2.0f, 0.5f);
    }

    #region Button Events
    public void PrintDepthEvent()
    {
#if ENABLE_WINMD_SUPPORT
        textOut.text = researchMode.GetCenterDepth().ToString();
#endif
    }

    public void PrintDepthExtrinsicsEvent()
    {
#if ENABLE_WINMD_SUPPORT
        textOut.text = researchMode.PrintDepthExtrinsics();
#endif
    }

    public void StartDepthSensingLoopEvent()
    {
#if ENABLE_WINMD_SUPPORT
        researchMode.StartDepthSensorLoop();
#endif
    }

    public void StopSensorLoopEvent()
    {
#if ENABLE_WINMD_SUPPORT
        researchMode.StopAllSensorDevice();
#endif
    }

    bool startRealtimePreview = true;
    public void StartPreviewEvent()
    {
        startRealtimePreview = !startRealtimePreview;
    }
    #endregion

    private void LateUpdate()
    {
        Process();
}

    private void Process()
    {
#if ENABLE_WINMD_SUPPORT
        // update depth map texture
        //if (startRealtimePreview && researchMode.DepthMapTextureUpdated())



        if (preview)
        {

            
            if (!mediaTexture)
            {
                mediaTexture = new Texture2D(640, 480);
                mediaMaterial.mainTexture = mediaTexture;
                textureInitialized = true;
            }
            if (!mediaTexture2)
            {
                mediaTexture2 = new Texture2D(640, 480);
                mediaMaterial2.mainTexture = mediaTexture2;
                textureInitialized = true;
            }
            if (!mediaTexture3)
            {
                mediaTexture3 = new Texture2D(Width, Height);
                mediaMaterial3.mainTexture = mediaTexture3;
                textureInitialized = true;
            }
            if (!mediaTexture4)
            {
                mediaTexture4 = new Texture2D(Width, Height);
                mediaMaterial4.mainTexture = mediaTexture4;
                textureInitialized = true;
            }
        }
            if (IMUEnabled)
            {
                Acc = researchMode.GetAccelBuffer();
                Gyro = researchMode.GetGyroBuffer();
                Mag = researchMode.GetMagBuffer();
                IMUUpdated = true;
            }

        //var B = researchMode.GetTemp();
        //textOut.text = " A:" + Acc[0] + " G:" + Gyro[0] + " M:" + Mag[0];



        if (GrayScale1Enabled)
        {
            if (researchMode.LFCameraTextureUpdated())
            {
                GrayScale1frameData = researchMode.GetCameraTextureBuffer(0);
                GrayScale1Updated = true;
                if (GrayScale1frameData != null && preview)
                {
                    Color[] pixels = new Color[640 * 480];
                    for (int i = 0; i < 480; i++)
                    {
                        for (int j = 0; j < 640; j++)
                        {
                            long idx = 640 * i + j;
                            pixels[idx] = new Color(GrayScale1frameData[idx] / 255f, GrayScale1frameData[idx] / 255f, GrayScale1frameData[idx] / 255f, 1);
                        }
                    }
                    mediaTexture.SetPixels(pixels);
                    mediaTexture.Apply();
                }
            }
        }
            


            //if (frameTexture.Length > 0)
            //{
            //    if (GrayScale1frameData == null)
            //    {
            //        GrayScale1frameData = frameTexture;

            //    }
            //    else
            //    {
            //        System.Buffer.BlockCopy(frameTexture, 0, GrayScale1frameData, 0, GrayScale1frameData.Length);
            //    }
            //    if (GrayScale1frameData != null && preview)
            //    {
            //        Color[] pixels = new Color[640 * 480];
            //        for (int i = 0; i < 480; i++)
            //        {
            //            for (int j = 0; j < 640; j++)
            //            {
            //                long idx = 640 * i + j;
            //                pixels[idx] = new Color(frameTexture[idx] / 255f, frameTexture[idx] / 255f, frameTexture[idx] / 255f, 1);
            //            }
            //        }
            //        mediaTexture.SetPixels(pixels);
            //        mediaTexture.Apply();
            //    }
            //}



            if(GrayScale2Enabled)
            {
                if (researchMode.RFCameraTextureUpdated())
                {
                    GrayScale2frameData = researchMode.GetCameraTextureBuffer(1);
                    GrayScale2Updated = true;

                    if (GrayScale2frameData != null && preview)
                    {
                        Color[] pixels = new Color[640 * 480];
                        for (int i = 0; i < 480; i++)
                        {
                            for (int j = 0; j < 640; j++)
                            {
                                long idx = 640 * i + j;
                                pixels[idx] = new Color(GrayScale2frameData[idx] / 255f, GrayScale2frameData[idx] / 255f, GrayScale2frameData[idx] / 255f, 1);
                            }
                        }
                        mediaTexture2.SetPixels(pixels);
                        mediaTexture2.Apply();
                    }
            }
        }

            //if (frameTexture.Length > 0)
            //{
            //    if (GrayScale2frameData == null)
            //    {
            //        GrayScale2frameData = frameTexture;
            //    }
            //    else
            //    {
            //        System.Buffer.BlockCopy(frameTexture, 0, GrayScale2frameData, 0, GrayScale2frameData.Length);
            //    }

            //    if (GrayScale2frameData != null && preview)
            //    {
            //        Color[] pixels = new Color[640 * 480];
            //        for (int i = 0; i < 480; i++)
            //        {
            //            for (int j = 0; j < 640; j++)
            //            {
            //                long idx = 640 * i + j;
            //                pixels[idx] = new Color(frameTexture[idx] / 255f, frameTexture[idx] / 255f, frameTexture[idx] / 255f, 1);
            //            }
            //        }
            //        mediaTexture2.SetPixels(pixels);
            //        mediaTexture2.Apply();
            //    }
            //}


            //if(Depth1Enabled)
            //{
            //    if (researchMode.DepthMapTextureUpdated())
            //    {
            //        Depth1frameData = researchMode.GetDepthMapTextureBuffer();
            //        Depth1Updated = true;
            //    }
            //}


            if (DepthEnabled)
            {
                if (researchMode.DepthLTMapTextureUpdated())
                {
                    Depth1frameData = researchMode.GetLTMapTextureBuffer();
                    Depth1Updated = true;
                    if (Depth1frameData != null && preview)
                        {
                        Color[] pixels = new Color[Width * Height];
                        for (int i = 0; i < Height; i++)
                        {
                            for (int j = 0; j < Width; j++)
                            {
                                long idx = Width * i + j;
                                pixels[idx] = new Color(Depth1frameData[idx] / 255f, Depth1frameData[idx] / 255f, Depth1frameData[idx] / 255f, 1);
                            }
                        }
                        mediaTexture3.SetPixels(pixels);
                        mediaTexture3.Apply();

                    }

                    Depth2frameData = researchMode.GetAbMapTextureBuffer();
                    Depth2Updated = true;
                    if (Depth2frameData != null && preview)
                        {
                        Color[] pixels = new Color[Width * Height];
                        for (int i = 0; i < Height; i++)
                        {
                            for (int j = 0; j < Width; j++)
                            {
                                long idx = Width * i + j;
                                pixels[idx] = new Color(Depth2frameData[idx] / 255f, Depth2frameData[idx] / 255f, Depth2frameData[idx] / 255f, 1);
                            }
                        }
                        mediaTexture4.SetPixels(pixels);
                        mediaTexture4.Apply();

                    }
                }
            }




        
#endif
    }

    public void IMUButtonTask()
    {
        Debug.Log("WTF");
        var tmp = GameObject.Find("RosSharp").GetComponent<IMUPublisher>();
        tmp.enabled = !tmp.enabled;

        IMUEnabled = !IMUEnabled;
        
    }
    public void GrayLButtonTask()
    {
        RawImagePublisher[] RawImagePublishers = GameObject.Find("RosSharp").GetComponents<RawImagePublisher>();
        foreach (RawImagePublisher rawImagePublisher in RawImagePublishers)
        {
            if (rawImagePublisher.Type == 1)
            {
                rawImagePublisher.enabled = !rawImagePublisher.enabled;
            }
        }
        GrayScale1Enabled = !GrayScale1Enabled;
    }
    public void GrayRButtonTask()
    {
        RawImagePublisher[] RawImagePublishers = GameObject.Find("RosSharp").GetComponents<RawImagePublisher>();
        foreach (RawImagePublisher rawImagePublisher in RawImagePublishers)
        {
            if (rawImagePublisher.Type == 2)
            {
                rawImagePublisher.enabled = !rawImagePublisher.enabled;
            }
        }
        GrayScale2Enabled = !GrayScale2Enabled;
    }
    public void DepthButtonTask()
    {
        RawImagePublisher[] RawImagePublishers = GameObject.Find("RosSharp").GetComponents<RawImagePublisher>();
        foreach (RawImagePublisher rawImagePublisher in RawImagePublishers)
        {
            if (rawImagePublisher.Type == 3)
            {
                rawImagePublisher.enabled = !rawImagePublisher.enabled;
            }
            if (rawImagePublisher.Type == 5)
            {
                rawImagePublisher.enabled = !rawImagePublisher.enabled;
            }
            if (rawImagePublisher.Type == 6)
            {
                rawImagePublisher.enabled = !rawImagePublisher.enabled;
            }
        }
        DepthEnabled = !DepthEnabled;
        
    }

    public void PreviewTask()
    {
        preview = !preview;
    }

    }