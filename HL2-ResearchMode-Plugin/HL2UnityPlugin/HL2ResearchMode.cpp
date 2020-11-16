#include "pch.h"
#include "HL2ResearchMode.h"
#include "HL2ResearchMode.g.cpp"

extern "C"
HMODULE LoadLibraryA(
    LPCSTR lpLibFileName
);

static ResearchModeSensorConsent camAccessCheck;
static ResearchModeSensorConsent imuAccessCheck;
static HANDLE camConsentGiven;
static HANDLE imuConsentGiven;

using namespace DirectX;
using namespace winrt::Windows::Perception;
using namespace winrt::Windows::Perception::Spatial;
using namespace winrt::Windows::Perception::Spatial::Preview;

typedef std::chrono::duration<int64_t, std::ratio<1, 10'000'000>> HundredsOfNanoseconds;

namespace winrt::HL2UnityPlugin::implementation
{
    void HL2ResearchMode::InitializeDepthSensor() 
    {
        // Load Research Mode library
        camConsentGiven = CreateEvent(nullptr, true, false, nullptr);
        imuConsentGiven = CreateEvent(nullptr, true, false, nullptr);
        HMODULE hrResearchMode = LoadLibraryA("ResearchModeAPI");
        HRESULT hr = S_OK;
        
        if (hrResearchMode)
        {
            typedef HRESULT(__cdecl* PFN_CREATEPROVIDER) (IResearchModeSensorDevice** ppSensorDevice);
            PFN_CREATEPROVIDER pfnCreate = reinterpret_cast<PFN_CREATEPROVIDER>(GetProcAddress(hrResearchMode, "CreateResearchModeSensorDevice"));
            if (pfnCreate)
            {
                winrt::check_hresult(pfnCreate(&m_pSensorDevice));
            }
            else
            {
                winrt::check_hresult(E_INVALIDARG);
            }
        }

        // get spatial locator of rigNode
        GUID guid;
        IResearchModeSensorDevicePerception* pSensorDevicePerception;
        winrt::check_hresult(m_pSensorDevice->QueryInterface(IID_PPV_ARGS(&pSensorDevicePerception)));
        winrt::check_hresult(pSensorDevicePerception->GetRigNodeId(&guid));
        pSensorDevicePerception->Release();
        m_locator = SpatialGraphInteropPreview::CreateLocatorForNode(guid);
        
        size_t sensorCount = 0;

        winrt::check_hresult(m_pSensorDevice->QueryInterface(IID_PPV_ARGS(&m_pSensorDeviceConsent)));
        winrt::check_hresult(m_pSensorDeviceConsent->RequestCamAccessAsync(HL2ResearchMode::CamAccessOnComplete));
        winrt::check_hresult(m_pSensorDeviceConsent->RequestIMUAccessAsync(HL2ResearchMode::ImuAccessOnComplete));
        
        m_pSensorDevice->DisableEyeSelection();

        winrt::check_hresult(m_pSensorDevice->GetSensorCount(&sensorCount));
        m_sensorDescriptors.resize(sensorCount);
        winrt::check_hresult(m_pSensorDevice->GetSensorDescriptors(m_sensorDescriptors.data(), m_sensorDescriptors.size(), &sensorCount));

        for (auto sensorDescriptor : m_sensorDescriptors)
        {
            IResearchModeCameraSensor* pCameraSensor = nullptr;

            if (sensorDescriptor.sensorType == LEFT_FRONT)
            {
                winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pLFCameraSensor));
                //winrt::check_hresult(m_pLFCameraSensor->QueryInterface(IID_PPV_ARGS(&pCameraSensor)));
                //winrt::check_hresult(pCameraSensor->GetCameraExtrinsicsMatrix(&m_LFCameraPose));
            }

            if (sensorDescriptor.sensorType == RIGHT_FRONT)
            {
                winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pRFCameraSensor));
               // winrt::check_hresult(m_pRFCameraSensor->QueryInterface(IID_PPV_ARGS(&pCameraSensor)));
                //winrt::check_hresult(pCameraSensor->GetCameraExtrinsicsMatrix(&m_RFCameraPose));
            }

#define DEPTH_USE_LONG_THROW
            //#undef  DEPTH_USE_LONG_THROW

#ifdef DEPTH_USE_LONG_THROW
            if (sensorDescriptor.sensorType == DEPTH_LONG_THROW)
            {
                winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_LTDepthSensor));
            }
#else
            if (sensorDescriptor.sensorType == DEPTH_AHAT)
            {
                winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_depthSensor));
                winrt::check_hresult(m_depthSensor->QueryInterface(IID_PPV_ARGS(&m_pDepthCameraSensor)));
                winrt::check_hresult(m_pDepthCameraSensor->GetCameraExtrinsicsMatrix(&m_depthCameraPose));
                m_depthCameraPoseInvMatrix = XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_depthCameraPose));
                //break;
            }
#endif
            if (sensorDescriptor.sensorType == IMU_ACCEL)
            {
                winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pAccelSensor));
            }
            if (sensorDescriptor.sensorType == IMU_GYRO)
            {
                winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pGyroSensor));
            }
            if (sensorDescriptor.sensorType == IMU_MAG)
            {
                winrt::check_hresult(m_pSensorDevice->GetSensor(sensorDescriptor.sensorType, &m_pMagSensor));
            }

        }

        m_AccelBuffer[0] = 0.0;
        m_AccelBuffer[1] = 1.0;
        m_AccelBuffer[2] = 2.0;
    }

    void HL2ResearchMode::StartDepthSensorLoop() 
    {
        //std::thread th1([this] {this->DepthSensorLoopTest(); });
        if (m_refFrame == nullptr) 
        {
            m_refFrame = m_locator.GetDefault().CreateStationaryFrameOfReferenceAtCurrentLocation().CoordinateSystem();
        }
        if (m_LTDepthSensor)
        {
            m_pLTDepthUpdateThread = new std::thread(HL2ResearchMode::DepthSensorDALoop, this);
        }
          
        if (m_depthSensor)
        {
            m_pDepthUpdateThread = new std::thread(HL2ResearchMode::DepthSensorLoop, this);
        }


        m_pLFCameraUpdateThread= new std::thread(HL2ResearchMode::LFCameraSensorLoop, this);
        m_pRFCameraUpdateThread = new std::thread(HL2ResearchMode::RFCameraSensorLoop, this);

        m_AccelUpdateThread = new std::thread(HL2ResearchMode::AccelSensorLoop, this);
        m_GyroUpdateThread = new std::thread(HL2ResearchMode::GyroSensorLoop, this);
        m_MagUpdateThread = new std::thread(HL2ResearchMode::MagSensorLoop, this);

    }
    void HL2ResearchMode::LFCameraSensorLoop(HL2ResearchMode* pHL2ResearchMode)
    {
        // prevent starting loop for multiple times
        if (!pHL2ResearchMode->m_LFCameraLoopStarted)
        {
            pHL2ResearchMode->m_LFCameraLoopStarted = true;
        }
        else {
            return;
        }

        pHL2ResearchMode->m_pLFCameraSensor->OpenStream();

        try
        {
            while (pHL2ResearchMode->m_LFCameraLoopStarted)
            {
                IResearchModeSensorFrame* pSensorFrame = nullptr;
                ResearchModeSensorResolution resolution;
                pHL2ResearchMode->m_pLFCameraSensor->GetNextBuffer(&pSensorFrame);

                // process sensor frame
                pSensorFrame->GetResolution(&resolution);
                pHL2ResearchMode->m_LFCameraResolution = resolution;

                IResearchModeSensorVLCFrame* pVLCFrame = nullptr;
                winrt::check_hresult(pSensorFrame->QueryInterface(IID_PPV_ARGS(&pVLCFrame)));

                size_t outBufferCount = 0;
                const UINT8* pImage = nullptr;
                pVLCFrame->GetBuffer(&pImage, &outBufferCount);
                pHL2ResearchMode->m_LFCameraBufferSize = outBufferCount;

                auto pTexture = std::make_unique<uint8_t[]>(outBufferCount);

                for (UINT i = 0; i < resolution.Height; i++)
                {
                    for (UINT j = 0; j < resolution.Width; j++)
                    {
                        auto idx = resolution.Width * i + j;
                        UINT16 depth = pImage[idx];

                        if (depth == 0) { pTexture.get()[idx] = 0; }
                        else { pTexture.get()[idx] = (uint8_t)((float)depth); }
                        
                    }
                }
            

                // save data
                {
                    std::lock_guard<std::mutex> l(pHL2ResearchMode->LFC);


                    // save raw depth map
                    if (!pHL2ResearchMode->m_LFCameraBuffer)
                    {
                        OutputDebugString(L"Create Space for depth map...\n");
                        pHL2ResearchMode->m_LFCameraBuffer = new UINT8[outBufferCount];
                    }
                    memcpy(pHL2ResearchMode->m_LFCameraBuffer, pTexture.get(), outBufferCount * sizeof(UINT8));

                    //// save pre-processed depth map texture (for visualization)
                    //if (!pHL2ResearchMode->m_depthMapTexture)
                    //{
                    //    OutputDebugString(L"Create Space for depth map texture...\n");
                    //    pHL2ResearchMode->m_depthMapTexture = new UINT8[outBufferCount];
                    //}
                    //memcpy(pHL2ResearchMode->m_depthMapTexture, pDepthTexture.get(), outBufferCount * sizeof(UINT8));
                }

                pHL2ResearchMode->m_LFCameraTextureUpdated = true;
                //pHL2ResearchMode->m_pointCloudUpdated = true;

                pTexture.reset();

                // release space
                if (pSensorFrame) {
                    pSensorFrame->Release();
                }
                if (pVLCFrame)
                {
                    pVLCFrame->Release();
                }

            }
        }
        catch (...) {}
        pHL2ResearchMode->m_pLFCameraSensor->CloseStream();
        pHL2ResearchMode->m_pLFCameraSensor->Release();
        pHL2ResearchMode->m_pLFCameraSensor = nullptr;
        pHL2ResearchMode->m_pRFCameraSensor->CloseStream();
        pHL2ResearchMode->m_pRFCameraSensor->Release();
        pHL2ResearchMode->m_pRFCameraSensor = nullptr;
        pHL2ResearchMode->m_pSensorDevice->Release();
        pHL2ResearchMode->m_pSensorDevice = nullptr;
        pHL2ResearchMode->m_pSensorDeviceConsent->Release();
        pHL2ResearchMode->m_pSensorDeviceConsent = nullptr;
    }
    void HL2ResearchMode::RFCameraSensorLoop(HL2ResearchMode* pHL2ResearchMode)
    {
        // prevent starting loop for multiple times
        if (!pHL2ResearchMode->m_RFCameraLoopStarted)
        {
            pHL2ResearchMode->m_RFCameraLoopStarted = true;
        }
        else {
            return;
        }

        pHL2ResearchMode->m_pRFCameraSensor->OpenStream();

        try
        {
            while (pHL2ResearchMode->m_RFCameraLoopStarted)
            {
                IResearchModeSensorFrame* pSensorFrame = nullptr;
                ResearchModeSensorResolution resolution;
                pHL2ResearchMode->m_pRFCameraSensor->GetNextBuffer(&pSensorFrame);

                // process sensor frame
                pSensorFrame->GetResolution(&resolution);
                pHL2ResearchMode->m_RFCameraResolution = resolution;

                IResearchModeSensorVLCFrame* pVLCFrame = nullptr;
                winrt::check_hresult(pSensorFrame->QueryInterface(IID_PPV_ARGS(&pVLCFrame)));

                size_t outBufferCount = 0;
                const UINT8* pImage = nullptr;
                pVLCFrame->GetBuffer(&pImage, &outBufferCount);
                pHL2ResearchMode->m_RFCameraBufferSize = outBufferCount;

                auto pTexture = std::make_unique<uint8_t[]>(outBufferCount);

                for (UINT i = 0; i < resolution.Height; i++)
                {
                    for (UINT j = 0; j < resolution.Width; j++)
                    {
                        auto idx = resolution.Width * i + j;
                        UINT16 depth = pImage[idx];

                        if (depth == 0) { pTexture.get()[idx] = 0; }
                        else { pTexture.get()[idx] = (uint8_t)((float)depth); }

                    }
                }


                // save data
                {
                    std::lock_guard<std::mutex> l(pHL2ResearchMode->RFC);


                    // save raw depth map
                    if (!pHL2ResearchMode->m_RFCameraBuffer)
                    {
                        OutputDebugString(L"Create Space for depth map...\n");
                        pHL2ResearchMode->m_RFCameraBuffer = new UINT8[outBufferCount];
                    }
                    memcpy(pHL2ResearchMode->m_RFCameraBuffer, pTexture.get(), outBufferCount * sizeof(UINT8));

                    //// save pre-processed depth map texture (for visualization)
                    //if (!pHL2ResearchMode->m_depthMapTexture)
                    //{
                    //    OutputDebugString(L"Create Space for depth map texture...\n");
                    //    pHL2ResearchMode->m_depthMapTexture = new UINT8[outBufferCount];
                    //}
                    //memcpy(pHL2ResearchMode->m_depthMapTexture, pDepthTexture.get(), outBufferCount * sizeof(UINT8));
                }

                pHL2ResearchMode->m_RFCameraTextureUpdated = true;
                //pHL2ResearchMode->m_pointCloudUpdated = true;

                pTexture.reset();

                // release space
                if (pSensorFrame) {
                    pSensorFrame->Release();
                }
                if (pVLCFrame)
                {
                    pVLCFrame->Release();
                }

            }
        }
        catch (...) {}
        pHL2ResearchMode->m_pLFCameraSensor->CloseStream();
        pHL2ResearchMode->m_pLFCameraSensor->Release();
        pHL2ResearchMode->m_pLFCameraSensor = nullptr;
        pHL2ResearchMode->m_pRFCameraSensor->CloseStream();
        pHL2ResearchMode->m_pRFCameraSensor->Release();
        pHL2ResearchMode->m_pRFCameraSensor = nullptr;
        pHL2ResearchMode->m_pSensorDevice->Release();
        pHL2ResearchMode->m_pSensorDevice = nullptr;
        pHL2ResearchMode->m_pSensorDeviceConsent->Release();
        pHL2ResearchMode->m_pSensorDeviceConsent = nullptr;
    }
    void HL2ResearchMode::DepthSensorLoop(HL2ResearchMode* pHL2ResearchMode)
    {
        // prevent starting loop for multiple times
        if (!pHL2ResearchMode->m_depthSensorLoopStarted)
        {
            pHL2ResearchMode->m_depthSensorLoopStarted = true;
        }
        else {
            return;
        }

        pHL2ResearchMode->m_depthSensor->OpenStream();
        
        try 
        {
            while (pHL2ResearchMode->m_depthSensorLoopStarted)
            {
                IResearchModeSensorFrame* pDepthSensorFrame = nullptr;
                ResearchModeSensorResolution resolution;
                pHL2ResearchMode->m_depthSensor->GetNextBuffer(&pDepthSensorFrame);

                // process sensor frame
                pDepthSensorFrame->GetResolution(&resolution);
                pHL2ResearchMode->m_depthresolution = resolution;
                
                IResearchModeSensorDepthFrame* pDepthFrame = nullptr;
                winrt::check_hresult(pDepthSensorFrame->QueryInterface(IID_PPV_ARGS(&pDepthFrame)));

                size_t outBufferCount = 0;
                const UINT16* pDepth = nullptr;
                pDepthFrame->GetBuffer(&pDepth, &outBufferCount);
                pHL2ResearchMode->m_depthbufferSize = outBufferCount;

                auto pDepthTexture = std::make_unique<uint8_t[]>(outBufferCount);
                std::vector<float> pointCloud;

                // get tracking transform
                ResearchModeSensorTimestamp timestamp;
                pDepthSensorFrame->GetTimeStamp(&timestamp);

                auto ts = PerceptionTimestampHelper::FromSystemRelativeTargetTime(HundredsOfNanoseconds(checkAndConvertUnsigned(timestamp.HostTicks)));
                auto transToWorld = pHL2ResearchMode->m_locator.TryLocateAtTimestamp(ts, pHL2ResearchMode->m_refFrame);
                if (transToWorld == nullptr)
                {
                    continue;
                }
                auto rot = transToWorld.Orientation();
                /*{
                    std::stringstream ss;
                    ss << rot.x << "," << rot.y << "," << rot.z << "," << rot.w << "\n";
                    std::string msg = ss.str();
                    std::wstring widemsg = std::wstring(msg.begin(), msg.end());
                    OutputDebugString(widemsg.c_str());
                }*/
                auto quatInDx = XMFLOAT4(rot.x, rot.y, rot.z, rot.w);
                auto rotMat = XMMatrixRotationQuaternion(XMLoadFloat4(&quatInDx));
                auto pos = transToWorld.Position();
                auto posMat = XMMatrixTranslation(pos.x, pos.y, pos.z);
                auto depthToWorld = pHL2ResearchMode->m_depthCameraPoseInvMatrix * rotMat * posMat;

                for (UINT i = 0; i < resolution.Height; i++)
                {
                    for (UINT j = 0; j < resolution.Width; j++)
                    {
                        auto idx = resolution.Width * i + j;
                        UINT16 depth = pDepth[idx];
                        depth = (depth > 4090) ? 0 : depth;

                        // back-project point cloud within Roi
                        if (i > pHL2ResearchMode->depthCamRoi.kRowLower*resolution.Height&& i < pHL2ResearchMode->depthCamRoi.kRowUpper * resolution.Height &&
                            j > pHL2ResearchMode->depthCamRoi.kColLower* resolution.Width&& j < pHL2ResearchMode->depthCamRoi.kColUpper * resolution.Width &&
                            depth > pHL2ResearchMode->depthCamRoi.depthNearClip && depth < pHL2ResearchMode->depthCamRoi.depthFarClip)
                        {
                            float xy[2] = { 0, 0 };
                            float uv[2] = { j, i };
                            pHL2ResearchMode->m_pDepthCameraSensor->MapImagePointToCameraUnitPlane(uv, xy);
                            auto pointOnUnitPlane = XMFLOAT3(xy[0], xy[1], 1);
                            auto tempPoint = (float)depth / 1000 * XMVector3Normalize(XMLoadFloat3(&pointOnUnitPlane));
                            // apply transformation
                            auto pointInWorld = XMVector3Transform(tempPoint, depthToWorld);

                            pointCloud.push_back(XMVectorGetX(pointInWorld));
                            pointCloud.push_back(XMVectorGetY(pointInWorld));
                            pointCloud.push_back(-XMVectorGetZ(pointInWorld));
                        }

                        // save as grayscale texture pixel into temp buffer
                        if (depth == 0) { pDepthTexture.get()[idx] = 0; }
                        else { pDepthTexture.get()[idx] = (uint8_t)((float)depth / 1000 * 255); }

                        // save the depth of center pixel
                        if (i == (UINT)(0.35 * resolution.Height) && j == (UINT)(0.5 * resolution.Width))
                        {
                            pHL2ResearchMode->m_depthcenterDepth = depth;
                            if (depth > pHL2ResearchMode->depthCamRoi.depthNearClip && depth < pHL2ResearchMode->depthCamRoi.depthFarClip)
                            {
                                std::lock_guard<std::mutex> l(pHL2ResearchMode->mu);
                                pHL2ResearchMode->m_depthcenterPoint[0] = *(pointCloud.end() - 3);
                                pHL2ResearchMode->m_depthcenterPoint[1] = *(pointCloud.end() - 2);
                                pHL2ResearchMode->m_depthcenterPoint[2] = *(pointCloud.end() - 1);
                            }
                            
                        }
                    }
                }

                // save data
                {
                    std::lock_guard<std::mutex> l(pHL2ResearchMode->mu);

                    // save point cloud
                    if (!pHL2ResearchMode->m_pointCloud)
                    {
                        OutputDebugString(L"Create Space for point cloud...\n");
                        pHL2ResearchMode->m_pointCloud = new float[outBufferCount * 3];
                    }

                    memcpy(pHL2ResearchMode->m_pointCloud, pointCloud.data(), pointCloud.size() * sizeof(float));
                    pHL2ResearchMode->m_pointcloudLength = pointCloud.size();

                    // save raw depth map
                    if (!pHL2ResearchMode->m_depthMap)
                    {
                        OutputDebugString(L"Create Space for depth map...\n");
                        pHL2ResearchMode->m_depthMap = new UINT16[outBufferCount];
                    }
                    memcpy(pHL2ResearchMode->m_depthMap, pDepth, outBufferCount * sizeof(UINT16));

                    // save pre-processed depth map texture (for visualization)
                    if (!pHL2ResearchMode->m_depthMapTexture)
                    {
                        OutputDebugString(L"Create Space for depth map texture...\n");
                        pHL2ResearchMode->m_depthMapTexture = new UINT8[outBufferCount];
                    }
                    memcpy(pHL2ResearchMode->m_depthMapTexture, pDepthTexture.get(), outBufferCount * sizeof(UINT8));
                }

                pHL2ResearchMode->m_depthMapTextureUpdated = true;
                pHL2ResearchMode->m_pointCloudUpdated = true;

                pDepthTexture.reset();

                // release space
                if (pDepthFrame) {
                    pDepthFrame->Release();
                }
                if (pDepthSensorFrame)
                {
                    pDepthSensorFrame->Release();
                }
                
            }
        }
        catch (...)  {}
        pHL2ResearchMode->m_depthSensor->CloseStream();
        pHL2ResearchMode->m_depthSensor->Release();
        pHL2ResearchMode->m_depthSensor = nullptr;
        pHL2ResearchMode->m_pSensorDevice->Release();
        pHL2ResearchMode->m_pSensorDevice = nullptr;
        pHL2ResearchMode->m_pSensorDeviceConsent->Release();
        pHL2ResearchMode->m_pSensorDeviceConsent = nullptr;
    }
    void HL2ResearchMode::DepthSensorDALoop(HL2ResearchMode* pHL2ResearchMode)
    {
        if (!pHL2ResearchMode->m_LTDepthLoopStarted)
        {
            pHL2ResearchMode->m_LTDepthLoopStarted = true;
        }
        else {
            return;
        }

        pHL2ResearchMode->m_LTDepthSensor->OpenStream();
        try
        {
            while (pHL2ResearchMode->m_LTDepthLoopStarted)
            {
                IResearchModeSensorFrame* pDepthSensorFrame = nullptr;
                ResearchModeSensorResolution resolution;
                pHL2ResearchMode->m_LTDepthSensor->GetNextBuffer(&pDepthSensorFrame);

                

                // process sensor frame
                pDepthSensorFrame->GetResolution(&resolution);
                pHL2ResearchMode->m_LTDepthResolution = resolution;
                pHL2ResearchMode->tmp = (int)resolution.Height;

                IResearchModeSensorDepthFrame* pDepthFrame = nullptr;
                winrt::check_hresult(pDepthSensorFrame->QueryInterface(IID_PPV_ARGS(&pDepthFrame)));

                size_t outBufferCount = 0;
                const BYTE* pSigma = nullptr;
                pDepthFrame->GetSigmaBuffer(&pSigma, &outBufferCount);
                //pHL2ResearchMode->tmp = pSigma[0];

                const UINT16* pDepth = nullptr;
                pDepthFrame->GetBuffer(&pDepth, &outBufferCount);
                pHL2ResearchMode->m_LTDepthBufferSize = outBufferCount;
                //pHL2ResearchMode->tmp = pDepth[0];

                const UINT16* pAbImage = nullptr;
                pDepthFrame->GetAbDepthBuffer(&pAbImage, &outBufferCount);
                //pHL2ResearchMode->m_LTDepthBufferSize = outBufferCount;

                auto pDepthTexture = std::make_unique<uint8_t[]>(outBufferCount);
                auto pAbTexture = std::make_unique<uint8_t[]>(outBufferCount);

                USHORT mask = 0x80;
                USHORT maxshort = 0;
                int maxClampDepth = 4000;
                for (UINT i = 0; i < resolution.Height; i++)
                {
                    for (UINT j = 0; j < resolution.Width; j++)
                    {
                        auto idx = resolution.Width * i + j;

                       
                        BYTE inputPixel = pHL2ResearchMode->ConvertDepthPixel(
                            pDepth[resolution.Width * i + j],
                            pSigma ? pSigma[resolution.Width * i + j] : 0,
                            mask,
                            maxshort,
                            0,
                            maxClampDepth);
                        pDepthTexture.get()[idx] = inputPixel;

                       
                        inputPixel = pHL2ResearchMode->ConvertDepthPixel(
                            pAbImage[resolution.Width * i + j],
                            pSigma ? pSigma[resolution.Width * i + j] : 0,
                            mask,
                            maxshort,
                            0,
                            maxClampDepth);
                        pAbTexture.get()[idx] = inputPixel;

                        //UINT16 depth = pDepth[idx];
                        //if (depth == 0) { pDepthTexture.get()[idx] = 0; }
                        //else { pDepthTexture.get()[idx] = (uint8_t)((float)depth / 1000 * 255); }

                        //UINT16 Ab = pAbImage[idx];
                        //if (Ab == 0) { pAbTexture.get()[idx] = 0; }
                        //else { pAbTexture.get()[idx] = (uint8_t)((float)Ab / 1000 * 255);}
                    }
                }
                //pHL2ResearchMode->tmp = pDepthTexture.get()[0];
                // save data
                {
                    
                    std::lock_guard<std::mutex> l(pHL2ResearchMode->LT);

                    if (!pHL2ResearchMode->m_LTDepthBuffer)
                    {
                        OutputDebugString(L"Create Space for depth map texture...\n");
                        pHL2ResearchMode->m_LTDepthBuffer = new UINT8[outBufferCount];
                    }
                    memcpy(pHL2ResearchMode->m_LTDepthBuffer, pDepthTexture.get(), outBufferCount * sizeof(UINT8));

                    if (!pHL2ResearchMode->m_LTDepthAbBuffer)
                    {
                        OutputDebugString(L"Create Space for Ab map texture...\n");
                        pHL2ResearchMode->m_LTDepthAbBuffer = new UINT8[outBufferCount];
                    }
                    memcpy(pHL2ResearchMode->m_LTDepthAbBuffer, pAbTexture.get(), outBufferCount * sizeof(UINT8));


                }

                pHL2ResearchMode->m_LTDepthTextureUpdated = true;
                //pHL2ResearchMode->m_AbMapTextureUpdated = true;
                //pHL2ResearchMode->m_pointCloudUpdated = true;

                pDepthTexture.reset();
                pAbTexture.reset();

                // release space
                if (pDepthFrame) {
                    pDepthFrame->Release();
                }
                if (pDepthSensorFrame)
                {
                    pDepthSensorFrame->Release();
                }

                         
                //pHL2ResearchMode->m_LTDepthSensor->CloseStream();


            }
            
        }
        catch (...) {}
        pHL2ResearchMode->m_LTDepthSensor->CloseStream();
        pHL2ResearchMode->m_LTDepthSensor->Release();
        pHL2ResearchMode->m_LTDepthSensor = nullptr;
        pHL2ResearchMode->m_pSensorDevice->Release();
        pHL2ResearchMode->m_pSensorDevice = nullptr;
        pHL2ResearchMode->m_pSensorDeviceConsent->Release();
        pHL2ResearchMode->m_pSensorDeviceConsent = nullptr;
    }




    void HL2ResearchMode::AccelSensorLoop(HL2ResearchMode* pHL2ResearchMode)
    {
        // prevent starting loop for multiple times
        if (!pHL2ResearchMode->m_AccelLoopStarted)
        {
            pHL2ResearchMode->m_AccelLoopStarted = true;
        }
        else {
            return;
        }

        pHL2ResearchMode->m_pAccelSensor->OpenStream();
        try
        {
            while (pHL2ResearchMode->m_AccelLoopStarted)
            {
                DirectX::XMFLOAT3 AccelSample;


                IResearchModeSensorFrame* pSensorFrame = nullptr;
                IResearchModeAccelFrame* pSensorAccelFrame = nullptr;


                ResearchModeSensorTimestamp timeStamp;
                const AccelDataStruct* pAccelBuffer = nullptr;
                size_t BufferOutLength;

               
                pHL2ResearchMode->m_pAccelSensor->GetNextBuffer(&pSensorFrame);
                pSensorFrame->QueryInterface(IID_PPV_ARGS(&pSensorAccelFrame));

                pSensorAccelFrame->GetCalibratedAccelaration(&AccelSample);

                //winrt::check_hresult(pSensorAccelFrame->GetCalibratedAccelarationSamples(&pAccelBuffer,&BufferOutLength));

                // save data
                {
                    std::lock_guard<std::mutex> l(pHL2ResearchMode->Accel);


                    // save raw depth map
                  

                    pHL2ResearchMode->m_AccelBuffer[0] = (float)AccelSample.x;
                    pHL2ResearchMode->m_AccelBuffer[1] = (float)AccelSample.y;
                    pHL2ResearchMode->m_AccelBuffer[2] = (float)AccelSample.z;
         /*           pHL2ResearchMode->m_AccelBuffer[0] = pAccelBuffer[0].AccelValues[0];

                    pHL2ResearchMode->m_AccelBuffer[1] = pAccelBuffer[0].AccelValues[1];

                    pHL2ResearchMode->m_AccelBuffer[2] = pAccelBuffer[0].AccelValues[2];*/



                   // memcpy(pHL2ResearchMode->m_RFCameraBuffer, pTexture.get(), outBufferCount * sizeof(UINT8));

                }

                pHL2ResearchMode->m_AccelUpdated = true;


                // release space
                if (pSensorFrame) {
                    pSensorFrame->Release();
                }
                if (pSensorAccelFrame)
                {
                    pSensorAccelFrame->Release();
                }


            }
        }
        catch (...) {}
        pHL2ResearchMode->m_pAccelSensor->CloseStream();
        pHL2ResearchMode->m_pAccelSensor->Release();
        pHL2ResearchMode->m_pAccelSensor = nullptr;
    }
    void HL2ResearchMode::GyroSensorLoop(HL2ResearchMode* pHL2ResearchMode)
    {
        // prevent starting loop for multiple times
        if (!pHL2ResearchMode->m_GyroLoopStarted)
        {
            pHL2ResearchMode->m_GyroLoopStarted = true;
        }
        else {
            return;
        }

        try
        {
            while (pHL2ResearchMode->m_GyroLoopStarted)
            {
                
                DirectX::XMFLOAT3 GyroSample;
                

                IResearchModeSensorFrame* pSensorFrame = nullptr;
                
                IResearchModeGyroFrame* pGyroFrame = nullptr;
               

                ResearchModeSensorTimestamp timeStamp;
                const AccelDataStruct* pAccelBuffer = nullptr;
                size_t BufferOutLength;

     
                pHL2ResearchMode->m_pGyroSensor->OpenStream();
                pHL2ResearchMode->m_pGyroSensor->GetNextBuffer(&pSensorFrame);
                pSensorFrame->QueryInterface(IID_PPV_ARGS(&pGyroFrame));
                pGyroFrame->GetCalibratedGyro(&GyroSample);
                

  

                // save data
                {
                    std::lock_guard<std::mutex> l(pHL2ResearchMode->Gyro);


                    // save raw depth map
  

                    pHL2ResearchMode->m_GyroBuffer[0] = GyroSample.x;
                    pHL2ResearchMode->m_GyroBuffer[1] = GyroSample.y;
                    pHL2ResearchMode->m_GyroBuffer[2] = GyroSample.z;


                    // memcpy(pHL2ResearchMode->m_RFCameraBuffer, pTexture.get(), outBufferCount * sizeof(UINT8));

                }

                pHL2ResearchMode->m_GyroUpdated = true;


                // release space
                if (pSensorFrame) {
                    pSensorFrame->Release();
                }

                if (pGyroFrame)
                {
                    pGyroFrame->Release();
                }


            }
        }
        catch (...) {}

        pHL2ResearchMode->m_pGyroSensor->CloseStream();
        pHL2ResearchMode->m_pGyroSensor->Release();
        pHL2ResearchMode->m_pGyroSensor = nullptr;

    }
    void HL2ResearchMode::MagSensorLoop(HL2ResearchMode* pHL2ResearchMode)
    {
        // prevent starting loop for multiple times
        if (!pHL2ResearchMode->m_MagLoopStarted)
        {
            pHL2ResearchMode->m_MagLoopStarted = true;
        }
        else {
            return;
        }

        try
        {
            while (pHL2ResearchMode->m_MagLoopStarted)
            {

                DirectX::XMFLOAT3 MagSample;

                IResearchModeSensorFrame* pSensorFrame = nullptr;

                IResearchModeMagFrame* pMagFrame = nullptr;

                ResearchModeSensorTimestamp timeStamp;
                const AccelDataStruct* pAccelBuffer = nullptr;
                size_t BufferOutLength;


                pHL2ResearchMode->m_pMagSensor->OpenStream();
                pHL2ResearchMode->m_pMagSensor->GetNextBuffer(&pSensorFrame);
                pSensorFrame->QueryInterface(IID_PPV_ARGS(&pMagFrame));
                pMagFrame->GetMagnetometer(&MagSample);
                

                // save data
                {
                    std::lock_guard<std::mutex> l(pHL2ResearchMode->Mag);


                    // save raw depth map
           

                    pHL2ResearchMode->m_MagBuffer[0] = MagSample.x;
                    pHL2ResearchMode->m_MagBuffer[1] = MagSample.y;
                    pHL2ResearchMode->m_MagBuffer[2] = MagSample.z;

                    // memcpy(pHL2ResearchMode->m_RFCameraBuffer, pTexture.get(), outBufferCount * sizeof(UINT8));

                }

                pHL2ResearchMode->m_MagUpdated = true;


                // release space
                if (pSensorFrame) {
                    pSensorFrame->Release();
                }

                if (pMagFrame)
                {
                    pMagFrame->Release();
                }

            }
        }
        catch (...) {}
        pHL2ResearchMode->m_pMagSensor->CloseStream();
        pHL2ResearchMode->m_pMagSensor->Release();
        pHL2ResearchMode->m_pMagSensor = nullptr;
    }



    void HL2ResearchMode::CamAccessOnComplete(ResearchModeSensorConsent consent)
    {
        camAccessCheck = consent;
        SetEvent(camConsentGiven);
    }
    void HL2ResearchMode::ImuAccessOnComplete(ResearchModeSensorConsent consent)
    {
        imuAccessCheck = consent;
        SetEvent(imuConsentGiven);
    }
    inline UINT16 HL2ResearchMode::GetCenterDepth() {return m_depthcenterDepth;}

    inline int HL2ResearchMode::GetBufferSize() { return m_LTDepthBufferSize; }
    inline UINT16 HL2ResearchMode::GetTemp() { return tmp; }

    inline bool HL2ResearchMode::DepthMapTextureUpdated() { return m_depthMapTextureUpdated; }
    inline bool HL2ResearchMode::DepthLTMapTextureUpdated() { return m_LTDepthTextureUpdated; }
    inline bool HL2ResearchMode::RFCameraTextureUpdated() { return m_RFCameraTextureUpdated; }
    inline bool HL2ResearchMode::LFCameraTextureUpdated() { return m_LFCameraTextureUpdated; }
    inline bool HL2ResearchMode::PointCloudUpdated() { return m_pointCloudUpdated; }

    hstring HL2ResearchMode::PrintResolution()
    {
        std::string res_c_ctr = std::to_string(m_depthresolution.Height) + "x" + std::to_string(m_depthresolution.Width) + "x" + std::to_string(m_depthresolution.BytesPerPixel);
        return winrt::to_hstring(res_c_ctr);// m_depthresolution.Width
    }

    hstring HL2ResearchMode::PrintDepthExtrinsics()
    {
        std::stringstream ss;
        ss << "Extrinsics: \n" << MatrixToString(m_depthCameraPose);
        std::string msg = ss.str();
        std::wstring widemsg = std::wstring(msg.begin(), msg.end());
        OutputDebugString(widemsg.c_str());
        return winrt::to_hstring(msg);
    }

    std::string HL2ResearchMode::MatrixToString(DirectX::XMFLOAT4X4 mat)
    {
        std::stringstream ss;
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                ss << mat(i, j) << ",";
            }
            ss << "\n";
        }
        return ss.str();
    }
    
    // Stop the sensor loop and release buffer space.
    // Sensor object should be released at the end of the loop function
    void HL2ResearchMode::StopAllSensorDevice()
    {
        m_depthSensorLoopStarted = false;
        //m_pDepthUpdateThread->join();
        if (m_depthMap) 
        {
            delete[] m_depthMap;
            m_depthMap = nullptr;
        }
        if (m_depthMapTexture) 
        {
            delete[] m_depthMapTexture;
            m_depthMapTexture = nullptr;
        }
        if (m_AbMapTexture)
        {
            delete[] m_AbMapTexture;
            m_AbMapTexture = nullptr;
        }
        if (m_LTDepthBuffer)
        {
            delete[] m_LTDepthBuffer;
            m_LTDepthBuffer = nullptr;
        }
        if (m_LTDepthAbBuffer)
        {
            delete[] m_LTDepthAbBuffer;
            m_LTDepthAbBuffer = nullptr;
        }
        if (m_pointCloud) 
        {
            m_pointcloudLength = 0;
            delete[] m_pointCloud;
            m_pointCloud = nullptr;
            
        }
      
    }

    com_array<uint16_t> HL2ResearchMode::GetDepthMapBuffer()
    {
        std::lock_guard<std::mutex> l(mu);
        if (!m_depthMap)
        {
            return com_array<uint16_t>();
        }
        com_array<UINT16> tempBuffer = com_array<UINT16>(m_depthMap, m_depthMap + m_depthbufferSize);
        
        return tempBuffer;
    }

    // Get depth map texture buffer. (For visualization purpose)
    com_array<uint8_t> HL2ResearchMode::GetDepthMapTextureBuffer()
    {
        std::lock_guard<std::mutex> l(mu);
        if (!m_depthMapTexture) 
        {
            return com_array<UINT8>();
        }
        com_array<UINT8> tempBuffer = com_array<UINT8>(std::move_iterator(m_depthMapTexture), std::move_iterator(m_depthMapTexture + m_depthbufferSize));

        m_depthMapTextureUpdated = false;
        return tempBuffer;
    }

    com_array<uint8_t> HL2ResearchMode::GetLTMapTextureBuffer()
    {
        std::lock_guard<std::mutex> l(LT);
        if (!m_LTDepthBuffer)
        {
            return com_array<UINT8>();
        }
        com_array<UINT8> tempBuffer = com_array<UINT8>(std::move_iterator(m_LTDepthBuffer), std::move_iterator(m_LTDepthBuffer + m_LTDepthBufferSize));

        m_LTDepthTextureUpdated = false;
        return tempBuffer;
    }
    com_array<uint8_t> HL2ResearchMode::GetAbMapTextureBuffer()
    {
        std::lock_guard<std::mutex> l(LT);
        if (!m_LTDepthAbBuffer)
        {
            return com_array<UINT8>();
        }
        com_array<UINT8> tempBuffer = com_array<UINT8>(std::move_iterator(m_LTDepthAbBuffer), std::move_iterator(m_LTDepthAbBuffer + m_LTDepthBufferSize));

        m_LTDepthTextureUpdated = false;
        return tempBuffer;
    }


    com_array<uint8_t> HL2ResearchMode::GetCameraTextureBuffer(int a)
    {
        if(a==0)
        {
            std::lock_guard<std::mutex> l(LFC);
            if (!m_LFCameraBuffer)
            {
                return com_array<UINT8>();
            }
            com_array<UINT8> tempBuffer = com_array<UINT8>(std::move_iterator(m_LFCameraBuffer), std::move_iterator(m_LFCameraBuffer + m_LFCameraBufferSize));

            m_LFCameraTextureUpdated = false;
            return tempBuffer;
        }
        else if (a == 1)
        {
            std::lock_guard<std::mutex> l(RFC);
            if (!m_RFCameraBuffer)
            {
                return com_array<UINT8>();
            }
            com_array<UINT8> tempBuffer = com_array<UINT8>(std::move_iterator(m_RFCameraBuffer), std::move_iterator(m_RFCameraBuffer + m_RFCameraBufferSize));

            m_RFCameraTextureUpdated = false;
            return tempBuffer;
        }
        
    }

    com_array<float> HL2ResearchMode::GetAccelBuffer()
    {
        std::lock_guard<std::mutex> l(Accel);

        com_array<float> tempBuffer = com_array<float>(std::move_iterator(m_AccelBuffer), std::move_iterator(m_AccelBuffer + 3));

        m_AccelUpdated = false;
        return tempBuffer;
    }

    com_array<float> HL2ResearchMode::GetGyroBuffer()
    {
        std::lock_guard<std::mutex> l(Gyro);

        com_array<float> tempBuffer = com_array<float>(std::move_iterator(m_GyroBuffer), std::move_iterator(m_GyroBuffer + 3));

        m_GyroUpdated = false;
        return tempBuffer;
    }

    com_array<float> HL2ResearchMode::GetMagBuffer()
    {
        std::lock_guard<std::mutex> l(Mag);

        com_array<float> tempBuffer = com_array<float>(std::move_iterator(m_MagBuffer), std::move_iterator(m_MagBuffer + 3));

        m_MagUpdated = false;
        return tempBuffer;
    }



    // Get the buffer for point cloud in the form of float array.
    // There will be 3n elements in the array where the 3i, 3i+1, 3i+2 element correspond to x, y, z component of the i'th point. (i->[0,n-1])
    com_array<float> HL2ResearchMode::GetPointCloudBuffer()
    {
        std::lock_guard<std::mutex> l(mu);
        if (m_pointcloudLength == 0)
        {
            return com_array<float>();
        }
        com_array<float> tempBuffer = com_array<float>(std::move_iterator(m_pointCloud), std::move_iterator(m_pointCloud + m_pointcloudLength));
        m_pointCloudUpdated = false;
        return tempBuffer;
    }

    // Get the 3D point (float[3]) of center point in depth map. Can be used to render depth cursor.
    com_array<float> HL2ResearchMode::GetCenterPoint()
    {
        std::lock_guard<std::mutex> l(mu);
        com_array<float> centerPoint = com_array<float>(std::move_iterator(m_depthcenterPoint), std::move_iterator(m_depthcenterPoint + 3));

        return centerPoint;
    }

    // Set the reference coordinate system. Need to be set before the sensor loop starts; otherwise, default coordinate will be used.
    void HL2ResearchMode::SetReferenceCoordinateSystem(winrt::Windows::Perception::Spatial::SpatialCoordinateSystem refCoord)
    {
        m_refFrame = refCoord;
    }

    long long HL2ResearchMode::checkAndConvertUnsigned(UINT64 val)
    {
        assert(val <= kMaxLongLong);
        return static_cast<long long>(val);
    }


    BYTE HL2ResearchMode::ConvertDepthPixel(USHORT v, BYTE bSigma, USHORT mask, USHORT maxshort, const int vmin, const int vmax)
    {
        if ((mask != 0) && (bSigma & mask) > 0)
        {
            v = 0;
        }

        if ((maxshort != 0) && (v > maxshort))
        {
            v = 0;
        }

        float colorValue = 0.0f;
        if (v <= vmin)
        {
            colorValue = 0.0f;
        }
        else if (v >= vmax)
        {
            colorValue = 1.0f;
        }
        else
        {
            colorValue = (float)(v - vmin) / (float)(vmax - vmin);
        }

        return (BYTE)(colorValue * 255);
    }



    
}
