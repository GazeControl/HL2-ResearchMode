# HL2-ResearchMode
### This is a Hololens2 app with following futures:
1. get raw sensor data from ResearchMode API plugin and stream to Ros.
2. Scan a room and build a pointcloud sandbox.
3. Display a bounding box which indicates the position of a robot.
4. Send control command to a robot by draging a marker.

# Get started
This tutorials below will show you how to deploy the Unity sample app to HL2 device.

## What you need before deploying
1. Visual Studio with UWP development tools and Unity development tools installed.
2. Unity 2019.4.2f1 in Windows10 system with UWP development tools installed.(Unity in Mac does not work with UWP project)
3. Ros melodic in ubuntu system as a server.
4. Research Mode for HoloLens 2 is available beginning with build 19041.1356. So windows insider program is needed to update the system version of HL2. After updating the system, turn on research mode switch in device portal, which is a web GUI for Hololens2. More instructions about turning on research mode can be found at [this Microsoft official document.](https://docs.microsoft.com/en-us/windows/mixed-reality/develop/platform-capabilities-and-apis/research-mode) 
* More details about Installing tools for Hololens2 can be found at [this Microsoft official document.](https://docs.microsoft.com/en-us/windows/mixed-reality/develop/install-the-tools?tabs=unity)

## Deploy app to Hololens2 device
1. Open the project. Choose `Assets/Scenes/SampleScene.unity` as the scene.
2. You can change server IP address  in `RosSharp/RosConnector.cs` in the inspector. 
2. Build the Unity project. The output is a Visual Studio Solution (which you will then have to also build).
3. **Add new package and capability to `Package.appxmanifest` file in the VS2019 solution.** [Check the Microsoft official sample](https://github.com/microsoft/HoloLens2ForCV/blob/main/Samples/SensorVisualization/SensorVisualization/Package.appxmanifest) and add `Capability` and `Package` to your own file.  
4. Open the VS2019 solution generated by Unity. Follow [releted Microsoft doc](https://docs.microsoft.com/en-us/windows/mixed-reality/develop/platform-capabilities-and-apis/using-visual-studio) to use USB-C cable(or wifi) to connect the HL2 device to PC. Set "ARM64" and "release" to deploy the app to HL2 device. 

# Project guide
There are some unity gameobjects in the unity project, here is a short instruction aobut their features.
1. "Directional Light". Generated by MRTK tool.
2. "RawDataSource". Turn on or turn off data source here, 3 scripts are attached. 'RM Plugin' controls if you want or want types of reaserch mode raw data you want. 'Video Panel App' controls if you want to the RGB viseo stream. 'Spatial Point Cloud' controls if you want to scan a room and generate a pointcloud sandbox.
3. "MixedReadlityToolkit". Generated by MRTK tool, you can adjust some HL2 settings here.
4. "MixedReadlityPlayspace". Generated by MRTK tool, Main Camera is here. And some planes for data preview are under this gameobject. 
5. "RosSharp". Control connection to Ros here. Change server's IP or control what Ros topic you want to send or receive. Each Ros topic has a unique scirpt. 
6. "ButtonBar". Button for some data switch. For example, Turn on Spatial Mapping switch to activate spatialmapping and let it render a pointcloud mesh, turn it off once the room pointcloud is established. 
7. "World". Some markers, bounding boxes and sripts about showing positions of robots, or calculating relative position and then send command to Ros. There is a Coordinate frame model and move it to align the real world famre in your room, then the position provided by HL2 will align to some other locolization system in your room(such as Vicon)
8. "Sandbox". Same as "World", but everything is in the room pointcloud sandbox.
## Example1: Stream sensor data to Ros
1. Load Ros package to ubuntu system. Make sure IP address of ubuntu is the same as what you set in Unity project
2. Lauch `roslaunch file_server ros_sharp_communication.launch`.
3. Turn off 'Spatialmapping' scripts, turn on 'RM Plugin' and 'Video Panel App' and choose what type of data you want. Activate preview and related gameobjects if you need.
4. According to the data type and you choose, activate related scripts in RosSharp
5. Build, deploy to HL2.
6. After server is running, then start your HL2 application. Now you might be able to find the Ros topics in ubuntu.
## Example2: Build a pointcloud room sandbox
1. Turn on 'Spatialmapping' scripts, turn off 'RM Plugin' and 'Video Panel App'.
2. Build, Deploy, start the app.
3. Turn on 'SpatialMapping' switch in the app. Move and scan your room. Turn it off once the whole room is scaned.

# Build plugin from source
1. Open plugin project using VS2019.
2. Make sure settings are set as `release` and `arm64`. Build the project.
3. Copy all output files to Unity project `'HL2-ResearchMode-Unity\Assets\ResearchMode\Plugins\ARM64'`. Make sure the platform is `WSAPlayer`, SDK is `UWP` and CPU is `ARM64` or `any CPU` .
* This plugin is a UWP windows runtime component pulgin, it only works in Hololens2 device. There is no way to use it in Unity Editor directly.

# Attention
1. If you are building the Unity project yourself, make sure to check the following setting.
   * Build platform is Universal Windows Platform.
   * Target and platform are set to `Hololens` and `ARM64`.
   * Windows Mixed Reality XR setting is activated in Player settings.
   * more details can be found at [related Microsoft doc.](https://docs.microsoft.com/en-us/windows/mixed-reality/develop/unity/configure-unity-project)
2. Be sure to add new package and capability to `Package.appxmanifest` file after the first build. The settings in the file will not be refreshed so you do not need to change it after the first time.
3. For researchMode API, there are two modes for depth raw data, Long Throw and AHAT. Only one mode is allowed at a time. If you want to change the mode, you need to go to plugin project and change #define from this to another, build from source and copy all output files to Unity project. [Check the Microsoft official samples for detail(raw 128)](https://github.com/microsoft/HoloLens2ForCV/blob/main/Samples/SensorVisualization/SensorVisualization/SensorVisualizationScenario.cpp)

# Content
There are 3 major parts in this project.
* HL2-ResearchMode-Plugin. This is the Visual Studio project for the Research Mode API plugin. You can build from source using this project.
* HL2-ResearchMode-Unity. Unity application project for Hololens2 device.
* Ros. Ros package as a server to receive messages published by HL2.

# Dependencies and reference
* [HoloLens2ForCV](https://github.com/microsoft/HoloLens2ForCV). Native C++ HL2 research mode API by Microsoft.
* [HoloLens2-ResearchMode-Unity](https://github.com/petergu684/HoloLens2-ResearchMode-Unity). 
* [HoloLensCameraStream](https://github.com/VulcanTechnologies/HoloLensCameraStream). The plugin that get the access to RGB camera video stream.
* [MRTK](https://github.com/microsoft/MixedRealityToolkit-Unity). Official Microsoft tools for Hololens

