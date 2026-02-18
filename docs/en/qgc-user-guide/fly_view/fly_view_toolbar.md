# Fly View Toolbar

![Fly View](../../../assets/fly/toolbar/fly_view_toolbar.jpg)

## Views

The "Q" icon on the left of the toolbar allows you to select between additional top level views:

- **[Plan Flight](../plan_view/plan_view.md):** Used to create missions, geo-fences and rally points
- **Analyze Tools:** A set of tools for things like log download, geo-tagging images, or viewing telemetry.
- **Vehicle Configuration:** The various options for the initial configuration of a new vehicle.
- **Application Settings:** Settings for the QGroundControl application itself.

## Toolbar Indicators

Next are multiple toolbar indicators for vehicle status. The dropdowns for each toolbar indicator provide additional detail on status. You can also expand the indicators to show additional application and vehicle settings associated with the indicator. Press the ">" button to expand.

![Toolbar Indicator - expand button](../../../assets/fly/toolbar_indicator_expand.png)

### Flight Status <img src="../../../assets/fly/toolbar/main_status_indicator.png" alt="Flight Status indicator" style="height: 1.15em; vertical-align: text-bottom;" />

The Flight Status indicator shows you whether the vehicle is ready to fly or not. It can be in one of the following states:

- **Ready To Fly** (_green background_) - Vehicle is ready to fly
- **Ready To Fly** (_yellow background_) - Vehicle is ready to fly in the current flight mode. But there are warnings which may cause problems.
- **Not Ready** - Vehicle is not ready to fly and will not takeoff.
- **Armed** - Vehicle is armed and ready to Takeoff.
- **Flying** - Vehicle is in the air and flying.
- **Landing** - Vehicle is in the process of landing.
- **Communication Lost** - QGroundControl has lost communication with the vehicle.

The Flight Status indicator dropdown also gives you access to:

- **Arm** - Arming a vehicle starts the motors in preparation for takeoff. You will only be able to arm the vehicle if it is safe and ready to fly. Generally you do not need to manually arm the vehicle. You can simply takeoff or start a mission and the vehicle will arm itself.
- **Disarm** - Disarming a vehicle is only available when the vehicle is on the ground. It will stop the motors. Generally you do not need to explicitly disarm as vehicles will disarm automatically after landing, or shortly after arming if you do not take off.
- **Emergency Stop** - Emergency stop is used to disarm the vehicle while it is flying. For emergency use only, your vehicle will crash!

In the cases of warnings or not ready state you can click the indicator to display the dropdown which will show the reason(s) why. The toggle on the right expands each error with additional information and possible solutions.

![UI To check arming warnings](../../../assets/fly/vehicle_states/arming_preflight_check_ui.png)

Once each issue is resolved it will disappear from the UI. When all issues blocking arming have been removed you should now be ready to fly.

### Flight Mode <img src="../../../assets/fly/toolbar/flight_modes_indicator.png" alt="Flight Mode indicator" style="height: 1.15em; vertical-align: text-bottom;" />

The Flight Mode indicator shows you the current flight mode. The dropdown allows you to switch between flight modes. The expanded page allows you to:

- Configure vehicle land settings
- Set global geo-fence settings
- Add/Remove flight modes from the displayed list

### Vehicle Messages <img src="../../../assets/fly/toolbar/messages_indicator.png" alt="Vehicle Messages indicator" style="height: 1.15em; vertical-align: text-bottom;" />

The Vehicle Messages indicator dropdown shows you messages which come from the vehicle. The indicator will turn red if there are important messages available.

### GPS / RTK GPS <img src="../../../assets/fly/toolbar/gps_indicator.png" alt="GPS / RTK GPS indicator" style="height: 1.15em; vertical-align: text-bottom;" />

The GPS/RTK GPS indicator shows satellite and GNSS status in the toolbar, and the dropdown provides additional GPS details.

With an active vehicle, the indicator shows vehicle GPS information (for example, satellite count and HDOP), and the expanded page provides access to RTK-related settings.

When there is no active vehicle but RTK is connected, the indicator switches to RTK status so you can still monitor the correction link.

### GPS Resilience

The GPS Resilience indicator appears when the vehicle reports GPS resilience telemetry (authentication, spoofing, or jamming state). The dropdown provides summary status and per-GPS details when available.

### Battery <img src="../../../assets/fly/toolbar/battery_indicator.png" alt="Battery indicator" style="height: 1.15em; vertical-align: text-bottom;" />

The Battery indicator shows you a configurable colored battery icon for remaining charge. It can also be configured to show percent remaining, voltage or both. The expanded page allows you to:

- Set what value(s) you want displayed in the battery icon
- Configure the icon coloring
- Set up the low battery failsafe

### Remote ID <img src="../../../assets/fly/toolbar/remote_id_indicator.png" alt="Remote ID indicator" style="height: 1.15em; vertical-align: text-bottom;" />

The Remote ID indicator appears when Remote ID is available on the active vehicle. Its color indicates overall Remote ID health, and the dropdown shows Remote ID status details.

### ESC <img src="../../../assets/fly/toolbar/esc_indicator.png" alt="ESC indicator" style="height: 1.15em; vertical-align: text-bottom;" />

The ESC indicator appears when ESC telemetry is available from the vehicle. It shows overall ESC health and online motor count, and opens a detailed ESC status page.

### Joystick <img src="../../../assets/fly/toolbar/joystick_indicator.png" alt="Joystick indicator" style="height: 1.15em; vertical-align: text-bottom;" />

The Joystick indicator appears when a joystick/gamepad is detected. The dropdown shows device status and connection details.

### Telemetry RSSI <img src="../../../assets/fly/toolbar/telemetry_rssi_indicator.png" alt="Telemetry RSSI indicator" style="height: 1.15em; vertical-align: text-bottom;" />

The Telemetry RSSI indicator appears when telemetry signal information is available. It provides local/remote RSSI and additional radio link quality details.

### RC RSSI <img src="../../../assets/fly/toolbar/rc_rssi_indicator.png" alt="RC RSSI indicator" style="height: 1.15em; vertical-align: text-bottom;" />

The RC RSSI indicator appears when RC signal information is available. It shows current RC link strength and opens a page with RC RSSI details.

### Gimbal <img src="../../../assets/fly/toolbar/gimbal_indicator.png" alt="Gimbal indicator" style="height: 1.15em; vertical-align: text-bottom;" />

The Gimbal indicator appears in the toolbar when the vehicle supports the [MAVLink Gimbal Protocol v2](https://mavlink.io/en/services/gimbal_v2.html). It displays the current gimbal status and provides controls for gimbal operation.

#### Gimbal Status Display

The toolbar indicator shows:

* **Pitch angle** - Current pitch position in degrees
* **Yaw value** - Either local yaw (relative to vehicle heading) or azimuth (absolute earth frame), depending on settings
* **Yaw Lock status** - Visual indicator showing whether yaw lock is enabled or disabled
  * **Yaw Lock (Locked icon)** - Gimbal yaw is locked to earth frame (maintains absolute heading)
  * **Yaw Follow (Follow icon)** - Gimbal yaw follows vehicle body frame (rotates with vehicle)

#### Gimbal Controls

Clicking the gimbal indicator opens a dropdown panel with the following controls:

##### Quick controls

* **Center** - Points the gimbal forward (0° pitch, 0° yaw relative to vehicle)
* **Tilt 90** - Points the gimbal straight down (-90° pitch)
* **Point Home** - Points the gimbal towards the home position
* **Retract** - Sets gimbal to retracted position. This is not supported by all gimbals.
* **Yaw Lock/Unlock** - More info below
* **Gimbal Control Acquisition** - More info below

##### Yaw Lock Mode Toggle

Toggle between two yaw control modes:

* **Yaw Lock** - Gimbal maintains a fixed heading relative to earth/north. When the vehicle rotates, the gimbal counter-rotates to maintain its absolute heading.
* **Yaw Follow** - Gimbal yaw is relative to the vehicle body. When the vehicle rotates, the gimbal rotates with it.

##### Gimbal Control Acquisition

If the option "Show Acquire/Release control button" is set, a new **Acquire/Release Control** button will appear
* Click to request control from the current operator, or to release control if we have it.
* Control is typically needed before sending gimbal commands. If not in control, **it will be requested automatically** when sending other commands.

#### Yaw Lock Behavior with Center and Tilt 90 Commands

By default, the Center and Tilt 90 buttons will also set yaw to follow mode. This is the typical user expectation for these commands.

**Example**: If you're in Yaw Lock mode and press "Center", the gimbal will point forward relative to the vehicle (Yaw Follow mode) rather than maintaining its absolute earth heading.

To change this behaviour and preserve last yaw lock status, you can enable the setting "Preserve yaw lock status on tilt 90 and center commands, don't force yaw follow".

### VTOL Transitions <img src="../../../assets/fly/toolbar/vtol_indicator.png" alt="VTOL indicator" style="height: 1.15em; vertical-align: text-bottom;" />

For VTOL vehicles, a VTOL transition status indicator is shown when applicable. It indicates the current VTOL mode/state and provides transition-related status information.

### MAVLink Signing <img src="../../../assets/fly/toolbar/signing_indicator.jpg" alt="MAVLink Signing indicator" style="height: 1.15em; vertical-align: text-bottom;" />

The MAVLink Signing indicator appears when signing keys have been configured (see [MAVLink 2 Signing](../settings_view/telemetry.md#signing)).
It shows a lock icon that indicates whether MAVLink 2 message signing is active on the current vehicle connection:

- **Locked (green):** Signing is active — the vehicle's incoming packets matched a stored key, or a key was manually enabled.
- **Unlocked:** Signing is not active on the current connection.

The dropdown shows the signing status, the name of the active key (if any), and the number of saved keys.
Expanding the indicator provides full key management: you can enable a key on the vehicle, disable the active key, delete unused keys, or add new keys.

### Multi-Vehicle Selector <img src="../../../assets/fly/toolbar/multi_vehicle_indicator.png" alt="Multi-Vehicle indicator" style="height: 1.15em; vertical-align: text-bottom;" />

The Multi-Vehicle selector appears when more than one vehicle is connected. It allows you to quickly switch the active vehicle from the toolbar.

### APM Support Forwarding <img src="../../../assets/fly/toolbar/apm_support_indicator.png" alt="APM Support Forwarding indicator" style="height: 1.15em; vertical-align: text-bottom;" />

On ArduPilot, an APM Support Forwarding indicator appears when MAVLink traffic forwarding to a support server is enabled.
