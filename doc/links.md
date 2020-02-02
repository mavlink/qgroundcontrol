# Link Management

The LinkManager creates, configures and maintains communication links. Links are created either through the user
interface or programmatically. The LinkConfiguration base classs defines the means to configure a given link
while the LinkInterface exposes the link itself.

Link specializations such as UDPLink, TCPLink, SerialLink, etc. are implemented in their own derived classes as well
as their equivalent configuration derivations such as UDPConfiguration, TCPConfiguration, SerialConfiguration, etc.

Links are primarily responsible to send and receive (MAVLink) data to and from a vehicle. When data arrives, the link will emit a
LinkInterface::bytesReceived signal and when data needs to be sent back to a vehicle, the code uses its
LinkInterface::writeBytesSafe method.

<div align="center">
<img src="../links.svg" style="width:80%; height=auto;">
</div>