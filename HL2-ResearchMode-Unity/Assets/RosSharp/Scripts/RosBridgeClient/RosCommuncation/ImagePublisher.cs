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



namespace RosSharp.RosBridgeClient
{
    public class ImagePublisher : UnityPublisher<MessageTypes.Sensor.CompressedImage>
    {
        //public Camera ImageCamera;
        public string FrameId = "Camera";
        public int resolutionWidth = 640;
        public int resolutionHeight = 480;
        //public Text Texto;
        [Range(0, 100)]
        public int qualityLevel = 50;

        private MessageTypes.Sensor.CompressedImage message;
        private Texture2D texture2D;
        private Rect rect;

        public GameObject previewPlane ;
        private Material mediaMaterial ;
        private Texture2D mediaTexture = null;
        private byte[] frameData = null;
        public RawImage rawImage;



        protected override void Start()
        {
            base.Start();
            //InitializeGameObject();
            InitializeMessage();


            
            //mediaMaterial = previewPlane.GetComponent<MeshRenderer>().material;
        }
        private void Update()
        {





            UpdateMessage();

        }

        // private void UpdateImage(Camera _camera)
        //{
        //    if (texture2D != null && _camera == this.ImageCamera)
        //        UpdateMessage();
        //}

        private void InitializeGameObject()
        {
           // texture2D = new Texture2D(resolutionWidth, resolutionHeight);
            //rect = new Rect(0, 0, resolutionWidth, resolutionHeight);
            //ImageCamera.targetTexture = new RenderTexture(resolutionWidth, resolutionHeight, 24);

            //mediaMaterial = previewPlane.GetComponent<MeshRenderer>().material;
            //texture2D= (Texture2D)mediaMaterial.mainTexture;
        }

        private void InitializeMessage()
        {
            message = new MessageTypes.Sensor.CompressedImage();
            message.header.frame_id = FrameId;
            message.format = "jpeg";
        }

        private void UpdateMessage()
        {
            message.header.Update();
            //texture2D=duplicateTexture((Texture2D)mediaMaterial.mainTexture);
            //message.data = texture2D.EncodeToJPG(qualityLevel);
            
            message.data= ImageConversion.EncodeToJPG(rawImage.texture as Texture2D, qualityLevel);
            //Texto.text = "UPdate~";
            Publish(message);
        }

        Texture2D duplicateTexture(Texture2D source)
        {
            RenderTexture renderTex = RenderTexture.GetTemporary(
                        source.width,
                        source.height,
                        0,
                        RenderTextureFormat.Default,
                        RenderTextureReadWrite.Linear);

            Graphics.Blit(source, renderTex);
            RenderTexture previous = RenderTexture.active;
            RenderTexture.active = renderTex;
            Texture2D readableText = new Texture2D(source.width, source.height);
            readableText.ReadPixels(new Rect(0, 0, renderTex.width, renderTex.height), 0, 0);
            readableText.Apply();
            RenderTexture.active = previous;
            RenderTexture.ReleaseTemporary(renderTex);
            return readableText;
        }

    }
}
