# ArduPilot Flight Mode Setup

The _Flight Modes_ section allows you to configure which flight modes and other actions are triggered by particular switches/switch positions on your RC transmitter.

::: info
In order to set up flight modes you must already have

> - In order to set up flight modes you must already have - [Configured your radio](../setup_view/radio.md) in order to set flight modes.
> - - [Setup the RC transmitter](../setup_view/FlightModes.md#transmitter-setup) (Flight Modes > Transmitter Setup)

To access this section, select the **Gear** icon (Vehicle Setup) in the top toolbar and then **Flight Modes** in the sidebar.

![Flight modes setup - ArduCopter](../../../assets/setup/flight_modes/ardupilot_copter.jpg)

## Flight Mode Settings

On ArduPilot you can assign up to 6 different flight modes to a single channel of your transmitter (the channel is selectable on Plane, but fixed to channel 5 on Copter).
ArduCopter also allows you to specify additional _Channel Options_ for channels 7-12.
These allow you to assign functions to these switches (for example, to turn on a camera, or return to launch).

To set the flight modes:

1. Turn on your RC transmitter.

2. Select the **Gear** icon (Vehicle Setup) in the top toolbar and then **Flight Modes** in the sidebar.

   ![Flight modes setup - ArduCopter](../../../assets/setup/flight_modes/ardupilot_copter.jpg)

   ::: info
   The above image is a screenshot of the flight mode setup for ArduCopter.
   :::

3. Select up to 6 flight modes in the drop downs.

4. **ArduCopter only:** Select additional _Channel Options_ for channels 7-12.

5. **ArduPlane only:** Select the mode channel from the dropdown.

   ![Flight modes setup - ArduPlane](../../../assets/setup/flight_modes/ardupilot_plane.jpg)

6. Test that the modes are mapped to the right transmitter switches by selecting each mode switch on your transmitter in turn, and check that the desired flight mode is activated (the text turns yellow on _QGroundControl_ for the active mode).

All values are automatically saved as they are changed.

The ArduCopter screenshot above shows a typical setup for a three position flight mode switch with an additional option of RTL being on a channel 7 switch.
You can also setup 6 flight modes using two switches plus mixing on your transmitter. Scroll down to the center section of this [page](http://ardupilot.org/copter/docs/common-rc-transmitter-flight-mode-configuration.html#common-rc-transmitter-flight-mode-configuration) for tutorials on how to do that.
:::

## See Also

- [ArduCopter Flight Modes](http://ardupilot.org/copter/docs/flight-modes.html)
- [ArduPlane Flight Modes](http://ardupilot.org/plane/docs/flight-modes.html)
- [ArduCopter > Auxiliary Function Switches](https://ardupilot.org/copter/docs/channel-7-and-8-options.html#channel-7-and-8-options) - additional information about channel configuration.
