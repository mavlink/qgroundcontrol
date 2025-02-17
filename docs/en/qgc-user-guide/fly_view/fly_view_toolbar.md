# Fly View Toolbar

## View Selector

The "Q" icon on the left of the toolbar allows you to select between additional top level views:

- **[Plan Flight](../plan_view/plan_view.md):** Used to create missions, geo-fences and rally points
- **Analyze Tools:** A set of tools for things like log download, geo-tagging images, or viewing telemetry.
- **Vehicle Configuration:** The various options for the initial configuration of a new vehicle.
- **Application Settings:** Settings for the QGroundControl application itself.

## Toolbar Indicators {#toolbar_indicators}

Next are a toolbar indicators for vehicle status. The dropdowns for each toolbar indicator provide additional detail on status. You can also expand the indicators to show additional application and vehicle settings associated with the indicator. Press the ">" button to expand.

![Toolbar Indicator - expand button](../../../assets/fly/toolbar_indicator_expand.png)

Here is an example expanded toolbar indicator for flight modes on a vehicle running PX4 firmware. The settings in this indicator provide access to things which may be relevant to change from flight to flight.

![Toolbar Indicator - expanded](../../../assets/fly/toolbar_indicator_expanded.png)

They also provide access to the Vehicle Configuration associated with the indicator. In this example: _Flight Modes_ - _Configure_.

### Ready/Not Ready Indicator

![Vehicle state - ready to fly green/ready background](../../../assets/fly/vehicle_states/ready_to_fly_ok.png)

Next in the toolbar is the indicator which shows you whether the vehicle is ready to fly or not.

It can be in one of the following states:

- **Ready To Fly** (_green background_) - Vehicle is ready to fly
- **Ready To Fly** (_yellow background_) - Vehicle is ready to fly in the current flight mode. But there are warnings which may cause problems.
- **Not Ready** - Vehicle is not ready to fly and will not takeoff.
- **Armed** - Vehicle is armed and ready to Takeoff.
- **Flying** - Vehicle is in the air and flying.
- **Landing** - Vehicle is in the process of landing.
- **Communication Lost** - QGroundControl has lost communication with the vehicle.

The Ready Indicator dropdown also gives you acess to:

- **Arming** - Arming a vehicle starts the motors in preparation for takeoff. You will only be able to arm the vehicle if it is safe and ready to fly. Generally you do not need to manually arm the vehicle. You can simply takeoff or start a mission and the vehicle will arm itself.
- **Disarm** - Disarming a vehicle is only availble when the vehicle is on the ground. It will stop the motors. Generally you do not need to explicitly disarm as vehicles will disarm automatically after landing, or shortly after arming if you do not take off.
- **Emergency Stop** - Emergency stop is effectively the same as disarming the vehicle while it is flying. For emergency use only, your vehicle will crash!

In the cases of warnings or not ready state you can click the indicator to display the dropdown which will show the reason(s) why. The toggle on the right expands each error with additional information and possible solutions.

![UI To check arming warnings](../../../assets/fly/vehicle_states/arming_preflight_check_ui.png)

Once each issue is resolved it will disappear from the UI.
When all issues blocking arming have been removed you should now be ready to fly.

## Flight Mode Indicator

![Vehicle state - ready to fly green/ready background](../../../assets/fly/toolbar/flight_modes_indicator.png)

The Flight Mode Indicator dropdown allows you to switch between flight modes. The expanded page allows you to:

- Configure vehicle land settings
- Set global geo-fence settings
- Add/Remove flight modes from the displayed list

## Vehicle Messages Indicator

![Vehicle state - ready to fly green/ready background](../../../assets/fly/toolbar/messages_indicator.png)

The Vehicle Messages Indicator dropdown shows you messages which come from the vehicle. The indicator will turn red if there are important messages available.

## GPS Indicator

![Vehicle state - ready to fly green/ready background](../../../assets/fly/toolbar/gps_indicator.png)

The GPS Indicator shows you the satellite count and the HDOP in the toolbar icon. The dropdown shows you additional GPS status. The expanded page give you access to RTK settings. 

## Battery Indicator

![Vehicle state - ready to fly green/ready background](../../../assets/fly/toolbar/battery_indicator.png)

The Battery Indicator shows you a configurable colored battery icon for remaining charge. It can also be configured to show percent remaining, voltage or both. The expanded page allows you to:

- Set what value(s) you want displayed in the battery icon
- Configure the icon coloring
- Set up the low battery failsafe