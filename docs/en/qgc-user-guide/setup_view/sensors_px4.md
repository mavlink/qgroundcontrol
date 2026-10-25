# Sensor setup (PX4)

::: tip Having trouble?
If parameters won't download after connecting, see [Parameter Download Failures](../troubleshooting/parameter_download.md). For connection issues, see [Connection Problems](../troubleshooting/vehicle_connection.md).
:::

The _Sensors_ page calibrates the vehicle's compass, gyroscope, accelerometer, barometer and, on fixed-wing and VTOL vehicles, airspeed sensor.
It also sets how the autopilot and compasses are mounted, which compass is the primary one, and which sensors preflight checks require.
It is designed for PX4 1.17 or later.

The page only shows settings the connected vehicle has, and only those that apply to its type.
The choices, units and limits of each setting come from the vehicle's parameter metadata.
Changes are written to the vehicle immediately.

At the top, a banner lists the sensors that need calibration before flight.
In each calibration row a green dot marks a sensor that is calibrated, a red dot one that needs calibrating before flight, and an empty dot one that is optional.
The _Live orientation_ card beside the rows shows a 3D model of the vehicle that follows its live roll, pitch and heading, and the sensors it found.
The red arrow on the circle under the model points north, so the nose points at it when the heading is 0.

The page has two tabs, _Calibration_ and _Advanced_.
Select _Advanced_ to show the automatic calibration, compass compensation, clipping notice and airspeed tubing rows, and the factory reset.

## Calibration

Click **Calibrate** to start a calibration.
The accelerometer and compass calibrations open a panel with a **Start** button.
Airspeed asks you to confirm in a dialog, and the others run in their row.
While a calibration runs the page shows its progress, and the rows below are locked.
**Cancel** stops it, and **Done** closes the panel once it has finished.

## Compass {#compass}

The process guides you to position the vehicle in a number of set orientations and rotate the vehicle about the specified axis.

The calibration steps are:

1. Click **Calibrate** in the _Compass_ row.

   ::: info
   The default flight controller orientation is _No rotation_ (flight controller and compass mounted upright on the vehicle and facing the front).
   You can set a different value on the panel or in the [Orientation](#flight_controller_orientation) section.
   :::

1. Click **Start** to start the calibration.
1. Place the vehicle on the side shown on the panel and hold it still.
   The panel suggests the sides in an order where each step is a single quarter turn, and upside down comes last.
   You can use the sides in any order.
1. When the vehicle shows turning, rotate the vehicle around the vertical axis (the side that points up) until the side turns green.
   Once the vehicle is detected on a side the panel keeps turning the vehicle until the autopilot has enough data for that side.
1. Repeat for all the sides.

Once all the sides are green the panel shows _Calibrated_.
Click **Reboot vehicle** to restart the vehicle, or reboot it yourself before flight.
If the calibration fails, click **Retry** to start over.
Some values may already have changed, so check that the sensor shows as calibrated before flight.

The picture of the vehicle shows the side to use next.
It is a 3D model of the vehicle type when the type is known, and a flat board otherwise.
When the graphics are not accelerated the picture is a still image.

**Auto-detect external compass rotation** sets the rotation of an external compass when you calibrate.
It compares the external compass with the internal one, so it needs both.
It does not work for a compass set to _Custom Euler Angle_.

**Large vehicle calibration…** is for vehicles that are too large or heavy to rotate through the orientations.
It sets the compass offsets from the earth's magnetic field at the vehicle's GPS position and the heading you give, so the vehicle does not need to be rotated.
Point the vehicle at a known heading (0 is true north, not magnetic north) and hold it still, then click **Calibrate** in the form on the panel.
It needs a GPS fix, and only the offsets are estimated.
Use the six side calibration instead when you can rotate the vehicle, because it is more reliable.
The offsets are saved when the vehicle disarms or reboots.

The row is hidden when the vehicle does not require any compass.

## Gyroscope {#gyroscope}

You will be guided to place the vehicle on a flat surface and keep it still.

1. Place the vehicle on a flat surface and keep it still.
1. Click **Calibrate** in the _Gyroscope_ row.

The calibration runs in the row and shows its progress.
When it finishes the row shows _Calibrated_.
If the vehicle moves, the autopilot restarts the calibration.
If it still can't finish, the row shows the reason and the button changes to **Retry**.

## Accelerometer {#accelerometer}

To calibrate the flight controller accelerometers you will be asked to place and hold your vehicle in a number of orientations (you will be prompted when to move between positions).

1. Click **Calibrate** in the _Accelerometer_ row.

   ::: info
   You should already have set the [Flight Controller Orientation](#flight_controller_orientation).
   If not, you can also set it on the panel.
   :::

1. Click **Start** to start the calibration.
1. Place the vehicle on the side shown on the panel and hold it still until the side turns green.
   This is very similar to compass calibration, but you only hold the vehicle still.
1. Repeat for all the sides.

Once all the sides are green the panel shows _Calibrated_.
If the calibration fails, click **Retry** to start over.
Some values may already have changed, so check that the sensor shows as calibrated before flight.

## Level horizon

If the horizon (as shown in the HUD) is not level after completing accelerometer calibration you can calibrate the level horizon for your vehicle.
It needs the gyroscope and accelerometer to be calibrated first.
The result is stored in the roll and pitch trims of the [Orientation](#flight_controller_orientation) section.

1. Place the vehicle in its level flight orientation on a level surface:

   - For planes this is the position during level flight (planes tend to have their wings slightly pitched up!)
   - For copters this is the hover position.

1. Keep the vehicle still and click **Calibrate** in the _Level horizon_ row.

The calibration runs in the row, shows its progress, and the row then shows _Calibrated_.
If the vehicle moves, or is too far from level, the row shows the reason and the button changes to **Retry**.

## Barometer {#barometer}

The barometer calibration matches the barometer to the GPS altitude, so the vehicle needs a 3D GPS fix.
The _Barometer_ row shows the GPS fix, and **Calibrate** is disabled until there is a 3D fix.
If the GPS height is only accurate to a few metres or worse, the row warns you but still lets you calibrate.

Keep the vehicle still and click **Calibrate**.
The calibration runs in the row, takes about 3 seconds, and the row then shows _Calibrated_.
If it fails, the row shows the reason and the button changes to **Retry**.

## Airspeed {#airspeed}

The _Airspeed_ row is shown on fixed-wing and VTOL vehicles which have an airspeed sensor.
The calibration needs to read a stable baseline with 0 airspeed to determine an offset.
Cup your hands over the airspeed sensor inlet to block any wind (if calibrating the sensor indoors this is not needed) and then blow into the tube using your mouth (to signal completion of the calibration).

To calibrate the airspeed sensor:

1. Click **Calibrate** in the _Airspeed_ row.

1. Cover the sensor (for example with your hand)

   ::: warning
   Do not block any holes in the sensor during calibration.
   :::

1. Click **OK** to start the calibration.
1. Blow into the sensor.
1. Wait for 2-3 seconds before removing the covering (calibration completes silently after several seconds)

## Compasses

A row is shown for each compass the vehicle has.
It names the sensor chip and whether it is internal or external, and sets:

- **Priority**: the compass with the highest priority is the primary one. Choosing the lowest setting turns the compass off.
- **Rotation**: the rotation of an external compass relative to the vehicle.
  Choose _Custom Euler Angle_ for a mounting that is not in the list, then enter the **Custom roll**, **Custom pitch**, and **Custom yaw** in degrees.

## Orientation {#flight_controller_orientation}

::: tip
You can skip this section if the flight controller and compass are mounted upright on the vehicle and facing the front (this is the default orientation - _No rotation_).
:::

If the autopilot/compass are mounted in any other way you will need to specify their orientations as YAW, PITCH and/or ROLL offsets relative to the forward-facing-upright orientation (clock-wise rotation around the Z, Y and X axis, respectively).

Set the **Rotation** of the _Autopilot_ row, and the _Rotation_ of each external compass in its row in [Compasses](#compasses).
Tilt and turn the vehicle and check the model in the _Live orientation_ card follows it the right way.
The **Roll trim**, **Pitch trim** and **Yaw trim** make small corrections on top of the rotation.

The _Autopilot_ row is hidden for vehicles whose sensors have a fixed orientation.

## Sensors

**Required sensors** sets which sensors preflight checks require: how many compasses, whether a barometer is needed, and, on fixed-wing and VTOL vehicles, whether an airspeed sensor is needed.
_Internal barometers_ turns off the autopilot's own barometers so an external one is used.
The compass and barometer settings apply after a reboot.

## Advanced

- **Automatic calibration**: lets the vehicle calibrate sensors itself.
  It sets an initial gyroscope, accelerometer, and compass calibration from the estimator's bias estimates when the sensor is not calibrated, and calibrates the barometer against the GNSS height.
- **Compass compensation**: removes the magnetic interference of the motors from the compass, based on either throttle or battery current.
- **Clipping notice**: notifies you when the IMU clips.
- **Airspeed tubing**: compensation for an SDP3x airspeed sensor, and reversing the readings when the static and dynamic ports are swapped.
- **Factory Reset**: clears the gyroscope, accelerometer, and compass calibration, then reboots the autopilot.
