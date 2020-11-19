using UnityEngine;


using System;

using HoloLensCameraStream;

public class MatrixUsageApp : MonoBehaviour
{
    byte[] _latestImageBytes;
    HoloLensCameraStream.Resolution _resolution;

    //"Injected" objects.
    GameObject _videoPanelUI;
    Renderer _videoPanelUIRenderer;
    Texture2D _videoTexture;
    HoloLensCameraStream.VideoCapture _videoCapture;
    IndicatorDisplay _targetIndicator;

    IntPtr _spatialCoordinateSystemPtr;

    void Start()
    {
        //Fetch a pointer to Unity's spatial coordinate system if you need pixel mapping
        _spatialCoordinateSystemPtr = UnityEngine.XR.WSA.WorldManager.GetNativeISpatialCoordinateSystemPtr();

        //Call this in Start() to ensure that the CameraStreamHelper is already "Awake".
        CameraStreamHelper.Instance.GetVideoCaptureAsync(OnVideoCaptureCreated);
        //You could also do this "shortcut":
        //CameraStreamManager.Instance.GetVideoCaptureAsync(v => videoCapture = v);

        _videoPanelUI = GameObject.CreatePrimitive(PrimitiveType.Quad);
        _videoPanelUI.name = "VideoPanelUI";
        _videoPanelUIRenderer = _videoPanelUI.GetComponent<Renderer>() as Renderer;
        _videoPanelUIRenderer.material = new Material(Shader.Find("AR/HolographicImageBlend"));

        _targetIndicator = GameObject.FindObjectOfType<IndicatorDisplay>();
    }

    private void OnDestroy()
    {
        if (_videoCapture != null)
        {
            _videoCapture.FrameSampleAcquired -= OnFrameSampleAcquired;
            _videoCapture.Dispose();
        }
    }

    void OnVideoCaptureCreated(HoloLensCameraStream.VideoCapture videoCapture)
    {
        if (videoCapture == null)
        {
            Debug.LogError("Did not find a video capture object. You may not be using the HoloLens.");
            return;
        }

        this._videoCapture = videoCapture;

        //Request the spatial coordinate ptr if you want fetch the camera and set it if you need to 
        CameraStreamHelper.Instance.SetNativeISpatialCoordinateSystemPtr(_spatialCoordinateSystemPtr);

        _resolution = CameraStreamHelper.Instance.GetLowestResolution();
        float frameRate = CameraStreamHelper.Instance.GetHighestFrameRate(_resolution);
        videoCapture.FrameSampleAcquired += OnFrameSampleAcquired;

        //You don't need to set all of these params.
        //I'm just adding them to show you that they exist.
        HoloLensCameraStream.CameraParameters cameraParams = new HoloLensCameraStream.CameraParameters();
        cameraParams.cameraResolutionHeight = _resolution.height;
        cameraParams.cameraResolutionWidth = _resolution.width;
        cameraParams.frameRate = Mathf.RoundToInt(frameRate);
        cameraParams.pixelFormat = HoloLensCameraStream.CapturePixelFormat.BGRA32;
        cameraParams.rotateImage180Degrees = false; //If your image is upside down, remove this line.
        cameraParams.enableHolograms = false;

        UnityEngine.WSA.Application.InvokeOnAppThread(() => { _videoTexture = new Texture2D(_resolution.width, _resolution.height, TextureFormat.BGRA32, false); }, false);

        videoCapture.StartVideoModeAsync(cameraParams, OnVideoModeStarted);
    }

    void OnVideoModeStarted(VideoCaptureResult result)
    {
        if (result.success == false)
        {
            Debug.LogWarning("Could not start video mode.");
            return;
        }

        Debug.Log("Video capture started.");
    }

    void OnFrameSampleAcquired(VideoCaptureSample sample)
    {
        //When copying the bytes out of the buffer, you must supply a byte[] that is appropriately sized.
        //You can reuse this byte[] until you need to resize it (for whatever reason).
        if (_latestImageBytes == null || _latestImageBytes.Length < sample.dataLength)
        {
            _latestImageBytes = new byte[sample.dataLength];
        }
        sample.CopyRawImageDataIntoBuffer(_latestImageBytes);

        //If you need to get the cameraToWorld matrix for purposes of compositing you can do it like this
        float[] cameraToWorldMatrixAsFloat;
        if (sample.TryGetCameraToWorldMatrix(out cameraToWorldMatrixAsFloat) == false)
        {
            return;
        }

        //If you need to get the projection matrix for purposes of compositing you can do it like this
        float[] projectionMatrixAsFloat;
        if (sample.TryGetProjectionMatrix(out projectionMatrixAsFloat) == false)
        {
            return;
        }

        // Right now we pass things across the pipe as a float array then convert them back into UnityEngine.Matrix using a utility method
        Matrix4x4 cameraToWorldMatrix = LocatableCameraUtils.ConvertFloatArrayToMatrix4x4(cameraToWorldMatrixAsFloat);
        Matrix4x4 projectionMatrix = LocatableCameraUtils.ConvertFloatArrayToMatrix4x4(projectionMatrixAsFloat);

        //This is where we actually use the image data
        //TODO: Create a class like VideoPanel for the next code
        UnityEngine.WSA.Application.InvokeOnAppThread(() =>
        {
            _videoTexture.LoadRawTextureData(_latestImageBytes);
            _videoTexture.wrapMode = TextureWrapMode.Clamp;
            _videoTexture.Apply();

            _videoPanelUIRenderer.sharedMaterial.SetTexture("_MainTex", _videoTexture);
            _videoPanelUIRenderer.sharedMaterial.SetMatrix("_WorldToCameraMatrix", cameraToWorldMatrix.inverse);
            _videoPanelUIRenderer.sharedMaterial.SetMatrix("_CameraProjectionMatrix", projectionMatrix);
            _videoPanelUIRenderer.sharedMaterial.SetFloat("_VignetteScale", 1.3f);

            
            Vector3 inverseNormal = -cameraToWorldMatrix.GetColumn(2);
            // Position the canvas object slightly in front of the real world web camera.
            Vector3 imagePosition = cameraToWorldMatrix.GetColumn(3) - cameraToWorldMatrix.GetColumn(2);

            _videoPanelUI.gameObject.transform.position = imagePosition;
            _videoPanelUI.gameObject.transform.rotation = Quaternion.LookRotation(inverseNormal, cameraToWorldMatrix.GetColumn(1));

        }, false);
    }
}
