/*
© CentraleSupelec, 2017
Author: Dr. Jeremy Fix (jeremy.fix@centralesupelec.fr)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
<http://www.apache.org/licenses/LICENSE-2.0>.
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// Adjustments to new Publication Timing and Execution Framework 
// © Siemens AG, 2018, Dr. Martin Bischoff (martin.bischoff@siemens.com)

using UnityEngine;
using UnityEngine.UI;

//using RMPlugin;



namespace RosSharp.RosBridgeClient
{
    public class RawImagePublisher : UnityPublisher<MessageTypes.Sensor.Image>
    {
        //public Camera ImageCamera;
        public string FrameId = "Camera";
        public int resolutionWidth = 640;
        public int resolutionHeight = 480;
        //public RMPlugin RMsource;
        //public Text textOut;
        private MessageTypes.Sensor.Image message;

        private byte[] frameData = null;

        public enum SensorType { GrayL, GrayR, Depth_AHAT, Depth_LT1, Depth_LT2,RGB };
        public SensorType sensorType;

        protected override void Start()
        {
            base.Start();
            //InitializeGameObject();
            InitializeMessage();



        }
        private void Update()
        {
            Process();
        }
        private void Process()
        {
            if(DataUpdated())
            { 
                UpdateMessage();
                ResetFlag();
            }
        }

       

        private void InitializeGameObject()
        {
            //texture2D = new Texture2D(resolutionWidth, resolutionHeight);
            ////rect = new Rect(0, 0, resolutionWidth, resolutionHeight);
            ////ImageCamera.targetTexture = new RenderTexture(resolutionWidth, resolutionHeight, 24);

            //mediaMaterial = previewPlane.GetComponent<MeshRenderer>().material;
            ////texture2D= (Texture2D)mediaMaterial.mainTexture;
        }

        private void InitializeMessage()
        {
            message = new MessageTypes.Sensor.Image();
            message.header.frame_id = FrameId;
            message.height = (uint)resolutionHeight;
            message.width = (uint)resolutionWidth;
            if (sensorType == SensorType.RGB)
            { message.encoding = "rgba8"; }
            else
            { message.encoding = "mono8"; }
               
            message.step = (uint)resolutionWidth;
            message.data = new byte[resolutionWidth * resolutionHeight];
        }

        private void UpdateMessage()
        {
            message.header.Update();
            

            if (sensorType == SensorType.GrayL)
            { message.data = RMPlugin.GrayScale1frameData; }
            else if (sensorType == SensorType.GrayR)
            { message.data = RMPlugin.GrayScale2frameData; }
            else if (sensorType == SensorType.Depth_AHAT)
            { message.data = RMPlugin.Depth1frameData; }
            else if (sensorType == SensorType.Depth_LT1)
            { message.data = RMPlugin.Depth1frameData; }
            else if (sensorType == SensorType.Depth_LT2)
            { message.data = RMPlugin.Depth2frameData; }
            else if (sensorType == SensorType.RGB)
            { message.data = VideoPanelApp._latestImageBytes; }


            Publish(message);
        }

        private bool DataUpdated()
        {
            if (sensorType == SensorType.GrayL)
            { return RMPlugin.GrayScale1Updated; }
            else if (sensorType == SensorType.GrayR)
            { return RMPlugin.GrayScale2Updated; }
            else if (sensorType == SensorType.Depth_AHAT)
            { return RMPlugin.Depth1Updated; }
            else if (sensorType == SensorType.Depth_LT1)
            { return RMPlugin.Depth1Updated; }
            else if (sensorType == SensorType.Depth_LT2)
            { return RMPlugin.Depth2Updated; }
            else if (sensorType == SensorType.RGB)
            { return VideoPanelApp.RGBUpdated; }
            else
            { return false; }
        }

        private void ResetFlag()
        {
            if (sensorType == SensorType.GrayL)
            { RMPlugin.GrayScale1Updated = false; }
            else if (sensorType == SensorType.GrayR)
            { RMPlugin.GrayScale2Updated = false; }
            else if (sensorType == SensorType.Depth_AHAT)
            { RMPlugin.Depth1Updated = false; }
            else if (sensorType == SensorType.Depth_LT1)
            { RMPlugin.Depth1Updated = false; }
            else if (sensorType == SensorType.Depth_LT2)
            { RMPlugin.Depth2Updated = false; }
            else if (sensorType == SensorType.RGB)
            { VideoPanelApp.RGBUpdated = true; }
            else
            { }
        }


    }
}
