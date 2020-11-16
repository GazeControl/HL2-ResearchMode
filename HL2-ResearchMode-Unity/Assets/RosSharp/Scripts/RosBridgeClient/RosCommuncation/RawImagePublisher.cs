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
        public int Type;
        private MessageTypes.Sensor.Image message;

        private byte[] frameData = null;



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
            if(DataUpdated(Type))
            { 
                UpdateMessage(Type);
                ResetFlag(Type);
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
            message.encoding = "mono8";
            message.step = (uint)resolutionWidth;
            message.data = new byte[resolutionWidth * resolutionHeight];
        }

        private void UpdateMessage(int m_Type)
        {
            message.header.Update();
            

            if (m_Type == 1)
            { message.data = RMPlugin.GrayScale1frameData; }
            else if (m_Type == 2)
            { message.data = RMPlugin.GrayScale2frameData; }
            else if (m_Type == 3)
            { message.data = RMPlugin.Depth1frameData; }
            else if (m_Type == 4)
            { }
            else if (m_Type == 5)
            { message.data = RMPlugin.Depth1frameData; }
            else if (m_Type == 6)
            { message.data = RMPlugin.Depth2frameData; }
            Publish(message);
        }

        private bool DataUpdated(int m_Type)
        {
            if (m_Type == 1)
            { return RMPlugin.GrayScale1Updated; }
            else if (m_Type == 2)
            { return RMPlugin.GrayScale2Updated; }
            else if (m_Type == 3)
            { return RMPlugin.Depth1Updated; }
            else if (m_Type == 4)
            { return false; }
            else if (m_Type == 5)
            { return RMPlugin.Depth1Updated; }
            else if (m_Type == 6)
            { return RMPlugin.Depth2Updated; }
            else
            { return false; }
        }

        private void ResetFlag(int m_Type)
        {
            if (m_Type == 1)
            { RMPlugin.GrayScale1Updated = false; }
            else if (m_Type == 2)
            { RMPlugin.GrayScale2Updated = false; }
            else if (m_Type == 3)
            { RMPlugin.Depth1Updated = false; }
            else if (m_Type == 4)
            { }
            else
            { }
        }


    }
}
