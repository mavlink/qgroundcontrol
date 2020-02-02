# Vehicle Management

The singleton MAVLinkProtocol implements the target slot for all LinkInterface::bytesReceived signals. As data is received, it parses and builds MAVLink messages.
All messages are then sent through a MAVLinkProcotol::messageReceived signal. In addition, when it detects a heartbeat message,
it emits MAVLinkProcotol::vehicleHeartbeatInfo signals. 

The singleton MultiVehicleManager is responsible for creating and maintaining instances of the Vehicle class. When it receives a MAVLinkProcotol::vehicleHeartbeatInfo
signal for the first time, it creates a vehicle instance, recording the vehicle ID and the link used. 

The Vehicle class holds all the functionality to handle vehicles. It receives all messages sent from the vehicle and manages all messages and commands to that vehicle.

<div align="center">
<img src="../vehicleMgmt.svg" style="width:80%; height=auto;">
</div>
