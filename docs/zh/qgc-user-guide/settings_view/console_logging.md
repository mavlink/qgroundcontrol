# Console Logging

The _Console_ can be helpful tool for diagnosing _QGroundControl_ problems. It can be found in **SettingsView > Console**.

![Console logging](../../../assets/support/console.jpg)

Click the **Set Logging** button to enable/disable logging information displayed by _QGroundControl_.

## Common Logging Options

The most commmonly used logging options are listed below.

| Option(s)                                                        | Description                                                                                                                    |
| ----------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| `LinkManagerLog`, `MultiVehicleManagerLog`                                          | Debug connection problems.                                                                                     |
| `LinkManagerVerboseLog`                                                             | Debug serial ports not being detected. Very noisy continuous output of available serial ports. |
| `FirmwareUpgradeLog`                                                                | Debug firmware flash issues.                                                                                   |
| `ParameterManagerLog`                                                               | Debug parameter load problems.                                                                                 |
| `ParameterManagerDebugCacheFailureLog`                                              | Debug parameter cache crc misses.                                                                              |
| `PlanManagerLog`, `MissionManagerLog`, `GeoFenceManagerLog`, `RallyPointManagerLog` | Debug Plan upload/download issues.                                                                             |
| `RadioComponentControllerLog`                                                       | Debug Radio calibration issues.                                                                                |

## Logging from the Command Line

An alternate mechanism for logging is using the `--logging` command line option. This is handy if you are trying to get logs from a situation where _QGroundControl_ crashes.

How you do this and where the traces are output vary by OS:

- Windows

  - You must open a command prompt, change directory to the **qgroundcontrol.exe** location, and run it from there:
    bash
    cd "\Program Files (x86)\qgroundcontrol"
    qgroundcontrol --logging:full

    ```sh
    cd "\Program Files (x86)\qgroundcontrol"
    qgroundcontrol --logging:full
    ```

  - When _QGroundControl_ starts you should see a separate console window open which will have the log output

- OSX

  - You must run _QGroundControl_ from Terminal. The Terminal app is located in Applications/Utilities. Once Terminal is open paste the following into it:

    ```sh
    Once Terminal is open paste the following into it:
        bash
        cd /Applications/qgroundcontrol.app/Contents/MacOS/
        ./qgroundcontrol --logging:full
    ```

  - Log traces will output to the Terminal window.

- Linux

  ```sh
  bash
  ./qgroundcontrol-start.sh --logging:full
  ```

  - Log traces will output to the shell you are running from.
