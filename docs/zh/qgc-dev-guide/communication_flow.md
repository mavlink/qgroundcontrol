# 通信流程

描述载具在自动连接期间进行的高级通信流程。

- LinkManager always has a UDP port open waiting for a vehicle heartbeat
- LinkManager detects a new known device type (Pixhawk, SiK Radio, PX4 Flow) that makes a UDP connection to the computer
  - LinkManager creates a new SerialLink between the computer and the device
- Incoming bytes from the Link are sent to MAVLinkProtocol
- MAVLinkProtocol将字节转换为MAVLink消息
- 如果消息是HEARTBEAT，则通知MultiVehicleManager(多机管理类)
- MultiVehicleManager is notified of the `HEARTBEAT` and creates a new vehicle object based on the information in the `HEARTBEAT` message
- The vehicle object instantiates the plugins that matches the vehicle
- The ParameterLoader associated with the vehicle object sends a `PARAM_REQUEST_LIST` to the connected device to load parameters using the parameter protocol
- Once the parameter load is complete, the MissionManager associated with the vehicle object requests the mission items from the connected device using the mission protocol
- 当参数加载完成后，VehicleComponents对象将在Setup视图中显示其UI
