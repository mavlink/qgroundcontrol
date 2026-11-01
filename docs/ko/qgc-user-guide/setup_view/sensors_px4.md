# Sensor setup (PX4)

:::tip Having trouble?
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

## 나침반 {#compass}

이 프로세스는 여러 설정된 방향으로 기체를 배치하고, 지정된 축을 중심으로 기체를 회전하도록 안내합니다.

보정 절차는 다음과 같습니다:

1. Click **Calibrate** in the _Compass_ row.

   ::: info
   The default flight controller orientation is _No rotation_ (flight controller and compass mounted upright on the vehicle and facing the front).
   You can set a different value on the panel or in the [Orientation](#flight_controller_orientation) section.
   :::

2. Click **Start** to start the calibration.

3. Place the vehicle on the side shown on the panel and hold it still.
   The panel suggests the sides in an order where each step is a single quarter turn, and upside down comes last.
   You can use the sides in any order.

4. When the vehicle shows turning, rotate the vehicle around the vertical axis (the side that points up) until the side turns green.
   Once the vehicle is detected on a side the panel keeps turning the vehicle until the autopilot has enough data for that side.

5. Repeat for all the sides.

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

## 자이로스코프 {#gyroscope}

차량을 평평한 곳에 놓은 채로 움직이지 않도록 합니다.

1. Place the vehicle on a flat surface and keep it still.
2. Click **Calibrate** in the _Gyroscope_ row.

The calibration runs in the row and shows its progress.
When it finishes the row shows _Calibrated_.
If the vehicle moves, the autopilot restarts the calibration.
If it still can't finish, the row shows the reason and the button changes to **Retry**.

## 가속도계 {#accelerometer}

To calibrate the flight controller accelerometers you will be asked to place and hold your vehicle in a number of orientations (you will be prompted when to move between positions).

1. Click **Calibrate** in the _Accelerometer_ row.

   ::: info
   You should already have set the [Flight Controller Orientation](#flight_controller_orientation).
   If not, you can also set it on the panel.
   :::

2. Click **Start** to start the calibration.

3. Place the vehicle on the side shown on the panel and hold it still until the side turns green.
   This is very similar to compass calibration, but you only hold the vehicle still.

4. Repeat for all the sides.

Once all the sides are green the panel shows _Calibrated_.
If the calibration fails, click **Retry** to start over.
Some values may already have changed, so check that the sensor shows as calibrated before flight.

## Level horizon

가속도계 보정 후, 수평선(HUD에 표시됨)이 수평이 아닌 경우 차량의 수평 수평을 보정할 수 있습니다.
It needs the gyroscope and accelerometer to be calibrated first.
The result is stored in the roll and pitch trims of the [Orientation](#flight_controller_orientation) section.

1. 기체를 방향을 유지한 채로 평평한 바닥에 두십시오:

   - 고정익의 경우 이 자세가 수평 비행 자세입니다 (비행기는 날개가 약간 올라가는 경향이 있습니다!).
   - For copters this is the hover position.

2. Keep the vehicle still and click **Calibrate** in the _Level horizon_ row.

The calibration runs in the row, shows its progress, and the row then shows _Calibrated_.
If the vehicle moves, or is too far from level, the row shows the reason and the button changes to **Retry**.

## Barometer {#barometer}

The barometer calibration matches the barometer to the GPS altitude, so the vehicle needs a 3D GPS fix.
The _Barometer_ row shows the GPS fix, and **Calibrate** is disabled until there is a 3D fix.
If the GPS height is only accurate to a few metres or worse, the row warns you but still lets you calibrate.

Keep the vehicle still and click **Calibrate**.
The calibration runs in the row, takes about 3 seconds, and the row then shows _Calibrated_.
If it fails, the row shows the reason and the button changes to **Retry**.

## 대기속도 {#airspeed}

The _Airspeed_ row is shown on fixed-wing and VTOL vehicles which have an airspeed sensor.
The calibration needs to read a stable baseline with 0 airspeed to determine an offset.
Cup your hands over the airspeed sensor inlet to block any wind (if calibrating the sensor indoors this is not needed) and then blow into the tube using your mouth (to signal completion of the calibration).

그런 다음, 다른 센서의 보정 작업을 진행합니다.

1. Click **Calibrate** in the _Airspeed_ row.

2. Cover the sensor (for example with your hand)

   ::: warning
   Do not block any holes in the sensor during calibration.
   :::

3. 보정을 시작하려면 **확인** 버튼을 클릭합니다.

4. 센서에 바람을 불어 넣습니다.

5. 덮개를 제거하기 전에 2-3 초간 기다립니다(몇 초 후 캘리브레이션이 자동으로 완료됩니다).

## Compasses

A row is shown for each compass the vehicle has.
It names the sensor chip and whether it is internal or external, and sets:

- **Priority**: the compass with the highest priority is the primary one. Choosing the lowest setting turns the compass off.
- **Rotation**: the rotation of an external compass relative to the vehicle.
  Choose _Custom Euler Angle_ for a mounting that is not in the list, then enter the **Custom roll**, **Custom pitch**, and **Custom yaw** in degrees.

## Orientation {#flight_controller_orientation}

:::tip
You can skip this section if the flight controller and compass are mounted upright on the vehicle and facing the front (this is the default orientation - _No rotation_).
:::

자동조종장치/나침반이 다른 방식으로 장착된 경우 해당 방향을 YAW, PITCH 및/또는 ROLL 오프셋으로 지정하여야 합니다(Z, Y 및 X축을 중심으로 시계 방향 회전)

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
