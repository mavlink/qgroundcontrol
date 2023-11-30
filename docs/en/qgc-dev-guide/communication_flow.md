# Communication Flow

Description of the high level communication flow which takes place during a vehicle auto-connect.

* LinkManager always has a UDP port open waiting for a vehicle heartbeat
* LinkManager detects a new known device type (Pixhawk, SiK Radio, PX4 Flow) that makes a UDP connection to the computer
    * LinkManager creates a new SerialLink between the computer and the device
* Incoming bytes from the Link are sent to MAVLinkProtocol
* MAVLinkProtocol converts the bytes into a MAVLink message
* If the message is a `HEARTBEAT` the MultiVehicleManager is notified
* MultiVehicleManager is notified of the `HEARTBEAT` and creates a new vehicle object based on the information in the `HEARTBEAT` message
* The vehicle object instantiates the plugins that matches the vehicle
* The ParameterLoader associated with the vehicle object sends a `PARAM_REQUEST_LIST` to the connected device to load parameters using the parameter protocol
* Once the parameter load is complete, the MissionManager associated with the vehicle object requests the mission items from the connected device using the mission protocol
* Once parameter load is complete, the VehicleComponents display their UI in the Setup view
