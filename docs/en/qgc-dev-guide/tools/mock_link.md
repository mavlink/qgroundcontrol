# Mock Link

_Mock Link_ allows you to create and stop links to multiple simulated (mock) vehicles in _QGroundControl_ debug builds.

The simulation does not support flight, but does allow easy testing of:

- Mission upload/download
- Viewing and changing parameters
- Testing most setup pages
- Multiple vehicle UIs

It is particularly useful for unit testing error cases for mission upload/download.

## Using Mock Link

To use _Mock Link_:

1. Create a Debug build by [building the source](../getting_started/index.md).
1. Access _Mock Link_ by selecting the _Application Settings_ icon in the top toolbar and then **Mock Link** in the sidebar.
1. Choose the **Vehicle Type** (PX4, ArduPilot Copter/Plane/Sub/Rover, or Generic) and **Home Location**, and optionally a served video stream and simulated camera, gimbal, proximity sensors or ADS-B vehicles.
1. Click **Start MockLink** to create a vehicle link with those settings.

   - Each time you click the button a new connection will be created.
   - When there is more than one connection the multiple-vehicle UI will appear.

1. Click **Stop One MockLink** to stop the currently active vehicle.

Using _Mock Link_ is then more or less the same as using any other vehicle, except that the simulation does not allow flight.

::: tip
To test with a vehicle that flies, use a SITL simulator. [`tools/simulation`](https://github.com/mavlink/qgroundcontrol/tree/master/tools/simulation) in the source tree has a lightweight MAVLink vehicle simulator and an ArduCopter SITL launcher.
:::
