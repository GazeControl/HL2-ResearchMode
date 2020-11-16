/*
© Siemens AG, 2017-2018
Author: Dr. Martin Bischoff (martin.bischoff@siemens.com)

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

// Added allocation free alternatives
// UoK , 2019, Odysseas Doumas (od79@kent.ac.uk / odydoum@gmail.com)

using UnityEngine;


namespace RosSharp.RosBridgeClient
{
    public class IMUPublisher : UnityPublisher<MessageTypes.Sensor.Imu>
    {
        public Transform PublishedTransform;

        public string FrameId = "Unity";

        private MessageTypes.Sensor.Imu message;

        protected override void Start()
        {
            base.Start();
            InitializeMessage();

        }
        private void Update()
        {
            Process();


        }

        private void Process()
        {
            if(RMPlugin.IMUUpdated==true)
            {
                UpdateMessage();
                RMPlugin.IMUUpdated = false;
            }
            
        }

        private void InitializeMessage()
        {
            message = new MessageTypes.Sensor.Imu
            {
                header = new MessageTypes.Std.Header()
                {
                    frame_id = FrameId
                }
            };

            for (int i = 0; i < 9; i++)
            {
                message.orientation_covariance[i] = -1;
                message.angular_velocity_covariance[i] = -1;
                message.linear_acceleration_covariance[i] = -1;
            }
        }

        private void UpdateMessage()
        {
            message.header.Update();
           
            GetGeometryQuaternion(PublishedTransform.rotation.Unity2Ros(), message.orientation);
            GetAccel(message.linear_acceleration);
            GetGyro(message.angular_velocity);
            Publish(message);

  
        }

        private static void GetGeometryQuaternion(Quaternion quaternion, MessageTypes.Geometry.Quaternion geometryQuaternion)
        {
            geometryQuaternion.x = quaternion.x;
            geometryQuaternion.y = quaternion.y;
            geometryQuaternion.z = quaternion.z;
            geometryQuaternion.w = quaternion.w;
        }

        private static void GetAccel(MessageTypes.Geometry.Vector3 geometryQuaternion)
        {
            geometryQuaternion.x = RMPlugin.Acc[0];
            geometryQuaternion.y = RMPlugin.Acc[1];
            geometryQuaternion.z = RMPlugin.Acc[2];
        }

        private static void GetGyro(MessageTypes.Geometry.Vector3 geometryQuaternion)
        {
            geometryQuaternion.x = RMPlugin.Gyro[0];
            geometryQuaternion.y = RMPlugin.Gyro[1];
            geometryQuaternion.z = RMPlugin.Gyro[2];
        }
    }
}
