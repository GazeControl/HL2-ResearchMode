#pragma once
#include "HL2ResearchMode.g.h"
#include "ResearchModeApi.h"
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <wchar.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <cmath>
#include <DirectXMath.h>
#include <vector>
#include<winrt/Windows.Perception.Spatial.h>
#include<winrt/Windows.Perception.Spatial.Preview.h>

namespace winrt::HL2UnityPlugin::implementation
{
    struct HL2ResearchMode : HL2ResearchModeT<HL2ResearchMode>
    {
        HL2ResearchMode() = default;

        UINT16 GetCenterDepth();
        int GetBufferSize();
        hstring PrintResolution();
        hstring PrintDepthExtrinsics();
        void InitializeDepthSensor();
        void StartDepthSensorLoop();
        void StopAllSensorDevice();
        bool DepthMapTextureUpdated();
        bool DepthLTMapTextureUpdated();
        bool RFCameraTextureUpdated();
        bool LFCameraTextureUpdated();
        bool PointCloudUpdated();
        void SetReferenceCoordinateSystem(Windows::Perception::Spatial::SpatialCoordinateSystem refCoord);
        BYTE ConvertDepthPixel(USHORT v, BYTE bSigma, USHORT mask, USHORT maxshort, const int vmin, const int vmax);
        com_array<uint16_t> GetDepthMapBuffer();
        com_array<uint8_t> GetDepthMapTextureBuffer();
        com_array<uint8_t> GetAbMapTextureBuffer();
        com_array<uint8_t> GetLTMapTextureBuffer();
        com_array<uint8_t> GetCameraTextureBuffer(int a);
        com_array<float> GetAccelBuffer();
        com_array<float> GetGyroBuffer();
        com_array<float> GetMagBuffer();
        UINT16 GetTemp();
        com_array<float> GetPointCloudBuffer();
        com_array<float> GetCenterPoint();
        std::mutex mu;
        std::mutex LT;
        std::mutex LFC;
        std::mutex RFC;
        std::mutex Accel;
        std::mutex Gyro;
        std::mutex Mag;

        
    private:
        float* m_pointCloud = nullptr;
        int m_pointcloudLength = 0;
        UINT16* m_depthMap = nullptr;
        UINT8* m_depthMapTexture = nullptr;
        UINT8* m_AbMapTexture = nullptr;
        IResearchModeSensor* m_depthSensor = nullptr;
        IResearchModeSensor* m_pLTSensor = nullptr;
        
        IResearchModeCameraSensor* m_pDepthCameraSensor = nullptr;
        ResearchModeSensorResolution m_depthresolution;
        IResearchModeSensorDevice* m_pSensorDevice = nullptr;
        std::vector<ResearchModeSensorDescriptor> m_sensorDescriptors;
        IResearchModeSensorDeviceConsent* m_pSensorDeviceConsent = nullptr;
        Windows::Perception::Spatial::SpatialLocator m_locator = 0;
        Windows::Perception::Spatial::SpatialCoordinateSystem m_refFrame = nullptr;
        std::atomic_int m_depthbufferSize = 0;
        std::atomic_int m_AbbufferSize = 0;
        std::atomic_uint16_t m_depthcenterDepth = 0;
        float m_depthcenterPoint[3];
        std::atomic_bool m_depthSensorLoopStarted = false;
        std::atomic_bool m_depthMapTextureUpdated = false;
        std::atomic_bool m_pointCloudUpdated = false;
        std::atomic_bool m_AbMapTextureUpdated = false;
        
        static void DepthSensorLoop(HL2ResearchMode* pHL2ResearchMode);
        static void DepthSensorDALoop(HL2ResearchMode* pHL2ResearchMode);
        static void LFCameraSensorLoop(HL2ResearchMode* pHL2ResearchMode);
        static void RFCameraSensorLoop(HL2ResearchMode* pHL2ResearchMode);
        static void AccelSensorLoop(HL2ResearchMode* pHL2ResearchMode);
        static void GyroSensorLoop(HL2ResearchMode* pHL2ResearchMode);
        static void MagSensorLoop(HL2ResearchMode* pHL2ResearchMode);

    

        static void CamAccessOnComplete(ResearchModeSensorConsent consent);
        static void ImuAccessOnComplete(ResearchModeSensorConsent consent);
        std::string MatrixToString(DirectX::XMFLOAT4X4 mat);
        DirectX::XMFLOAT4X4 m_depthCameraPose;
        DirectX::XMMATRIX m_depthCameraPoseInvMatrix;
        std::thread* m_pDepthUpdateThread;
        std::thread* m_pLFCameraUpdateThread;
        std::thread* m_pRFCameraUpdateThread;
        static long long checkAndConvertUnsigned(UINT64 val);
        struct DepthCamRoi {
            float kRowLower = 0.2;
            float kRowUpper = 0.5;
            float kColLower = 0.3;
            float kColUpper = 0.7;
            UINT16 depthNearClip = 200; // Unit: mm
            UINT16 depthFarClip = 800;
        } depthCamRoi;

        IResearchModeSensor* m_pLFCameraSensor = nullptr;
        std::atomic_bool m_LFCameraTextureUpdated = false;
        std::atomic_bool m_LFCameraLoopStarted = false;
        ResearchModeSensorResolution m_LFCameraResolution;
        UINT8* m_LFCameraBuffer = nullptr;
        std::atomic_int m_LFCameraBufferSize = 0;
        DirectX::XMFLOAT4X4 m_LFCameraPose;

        IResearchModeSensor* m_pRFCameraSensor = nullptr;
        ResearchModeSensorResolution m_RFCameraResolution;
        std::atomic_bool m_RFCameraTextureUpdated = false;
        std::atomic_bool m_RFCameraLoopStarted = false;
        UINT8* m_RFCameraBuffer = nullptr;
        std::atomic_int m_RFCameraBufferSize = 0;
        DirectX::XMFLOAT4X4 m_RFCameraPose;

        IResearchModeSensor* m_LTDepthSensor = nullptr;
        ResearchModeSensorResolution m_LTDepthResolution;
        std::atomic_bool m_LTDepthTextureUpdated = false;
        std::atomic_bool m_LTDepthLoopStarted = false;
        UINT8* m_LTDepthBuffer = nullptr;
        UINT8* m_LTDepthAbBuffer = nullptr;
        std::atomic_int m_LTDepthBufferSize = 0;
        std::thread* m_pLTDepthUpdateThread;

        float m_AccelBuffer[3];
        //float* m_AccelBuffer = nullptr;
        std::atomic_bool m_AccelLoopStarted = false;
        std::atomic_bool m_AccelUpdated = false;
        IResearchModeSensor* m_pAccelSensor = nullptr;
        std::thread* m_AccelUpdateThread;

        float m_GyroBuffer[3];
        //float* m_GyroBuffer = nullptr;
        std::atomic_bool m_GyroLoopStarted = false;
        std::atomic_bool m_GyroUpdated = false;
        IResearchModeSensor* m_pGyroSensor = nullptr;
        std::thread* m_GyroUpdateThread;

        float m_MagBuffer[3];
        //float* m_MagBuffer = nullptr;
        std::atomic_bool m_MagLoopStarted = false;
        std::atomic_bool m_MagUpdated = false;
        IResearchModeSensor* m_pMagSensor = nullptr;
        std::thread* m_MagUpdateThread;
            
        UINT16 tmp=233;
        
    };
}
namespace winrt::HL2UnityPlugin::factory_implementation
{
    struct HL2ResearchMode : HL2ResearchModeT<HL2ResearchMode, implementation::HL2ResearchMode>
    {
    };
}
