# Joystick Calibration failures

The most common joystick calibration failure is a stick that only reads half of its range: the axis monitor shows the stick resting at 50% and moving to 0% or 100%, but never both.
Calibration then stalls on a "move the stick all the way down/left" step because the stick never crosses the center point.

This is almost always an SDL controller-mapping problem rather than a hardware fault.
_QGroundControl_ reads joysticks through SDL's _gamepad_ interface, which maps each raw axis onto a standard gamepad layout (`leftx`, `lefty`, `rightx`, `righty`, `lefttrigger`, `righttrigger`).
Trigger outputs are one-sided (0 to +32767), so if SDL's built-in mapping assigns one of your sticks to `lefttrigger` or `righttrigger` that stick loses half its travel.

It mostly affects RC transmitters used as USB joysticks — EdgeTX/OpenTX radios such as the RadioMaster Zorro, Pocket, Boxer and TX16S, the FrSky Taranis and the TBS Tango 2 — because SDL has no curated mapping for them and generates one on the fly.
All reports so far have been on Linux, where SDL derives the generated mapping from the kernel axis codes and treats the radio's third axis as a trigger; it can occur on other platforms but is much less common.

## Confirming the problem

1. Open **Application Settings > Logging**, click **Categories**, and enable `Joystick.JoystickSDL` and `RemoteControl.RemoteControlCalibrationController`.
1. Quit _QGroundControl_, plug in the controller, and start _QGroundControl_ again so joystick discovery runs with logging on.
1. Go to **Vehicle Setup > Joystick > Calibrate**, press **Start**, and follow the steps until calibration stalls. Press **Cancel**.
1. Back in **Application Settings > Logging**, press **Save** and open the saved file.

Look for these lines from joystick discovery:

```text
Gamepad mapping for "OpenTX FrSky Taranis Joystick" : 030068cd09120000544f000011010000,OpenTX FrSky Taranis Joystick,...,rightx:a3,righty:a4,lefttrigger:a2,...
  Axis binding raw axis 2 [ -32768 .. 32767 ] -> lefttrigger [ 0 .. 32767 ] (trigger: half-range output)
Initial axis values: QList(0, 0, 0, 0, 16384, 0)
```

and these from the calibration attempt:

```text
Channel 4 rest value 16384 is not near center 0 - may be a trigger/one-sided axis
Calibration cancelled at step 2 function: "Throttle" mappedChannel: 4 observed range per channel this step: QList("-32768..32767", "-32768..32767", "-32768..32767", "-32768..32767", "0..32767", "n/a") ...
```

A raw axis bound to `lefttrigger`/`righttrigger` that rests near 16384 and only ever ranges 0..32767 while its neighbours span -32768..32767 confirms the mapping problem.
The first field of the mapping line (`030068cd...`) is the controller GUID you need for the fix.

## Fixing the mapping

Write a corrected mapping string that assigns your stick axes to `leftx`/`lefty`/`rightx`/`righty` and moves the trigger entries to axes you do not use.
Start from the mapping printed in the log and swap the axis numbers.
For the Taranis example above, `rightx:a3,righty:a4,lefttrigger:a2` becomes `rightx:a2,righty:a3,lefttrigger:a4`.

There are two ways to apply it:

- **Persistent (recommended):** add the mapping as a single line to _QGroundControl_'s user `gamecontrollerdb.txt` file (create the file if it does not exist). _QGroundControl_ loads this file at startup after its bundled database, so your entry overrides the built-in mapping.

  The exact path depends on your OS, build (stable vs daily) and configuration; it is printed in the `Joystick.JoystickSDL` log during joystick discovery:

  ```text
  User gamepad mapping file (create to override built-in mappings): "/home/user/.config/QGroundControl/QGroundControl/gamecontrollerdb.txt" ...
  ```

  The `platform:` field at the end of the mapping must match the OS the file is used on (`Linux`, `Mac OS X` or `Windows`).
  The corrected Taranis mapping from the example above would be:

  ```text
  030068cd09120000544f000011010000,OpenTX FrSky Taranis Joystick,a:b0,b:b1,x:b3,y:b4,back:b10,guide:b12,start:b11,leftstick:b13,rightstick:b14,leftshoulder:b6,rightshoulder:b7,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,crc:cd68,platform:Linux
  ```

- **One-off:** set the standard SDL `SDL_GAMECONTROLLERCONFIG` environment variable to the mapping string before launching _QGroundControl_ from a terminal. This is the workaround referenced in older forum posts and issues; it works but must be repeated every launch.

Restart _QGroundControl_ and repeat the calibration.
The `Gamepad mapping for ...` log line should now show your corrected mapping.

::: tip
If you get a mapping working for your radio, please open a pull request adding it to _QGroundControl_'s bundled `gamecontrollerdb.txt` so it works out of the box for others.
:::
