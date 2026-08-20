# Console Logging

The _App Log Viewer_ can be a helpful tool for diagnosing _QGroundControl_ problems. It can be found in **Application Settings > App Log Viewer**.

![App Log Viewer](../../../assets/support/app_log_viewer.jpg)

Click the **Categories** button to enable/disable the logging categories displayed by _QGroundControl_.

## Common Logging Categories

The most commonly used logging categories are listed below.

| Category(s)                                                                                                             | Description                                                                                    |
| ----------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------- |
| `Comms.LinkManager`, `Vehicle.MultiVehicleManager`                                                                      | Debug connection problems.                                                                     |
| `Comms.LinkManager:verbose`                                                                                             | Debug serial ports not being detected. Very noisy continuous output of available serial ports. |
| `VehicleSetup.FirmwareUpgrade`                                                                                          | Debug firmware flash issues.                                                                   |
| `FactSystem.ParameterManager`                                                                                           | Debug parameter load problems.                                                                 |
| `FactSystem.ParameterManager:debugCacheFailure`                                                                         | Debug parameter cache crc misses.                                                              |
| `PlanManager.PlanManager`, `PlanManager.MissionManager`, `PlanManager.GeoFenceManager`, `PlanManager.RallyPointManager` | Debug Plan upload/download issues.                                                             |
| `AutoPilotPlugins.RadioComponentController`                                                                             | Debug Radio calibration issues.                                                                |

## Logging from the Command Line

An alternate mechanism for logging is using the `--logging` command line option. This is handy if you are trying to get logs from a situation where _QGroundControl_ crashes.

How you do this and where the traces are output vary by OS:

- Windows

  - You must open a command prompt, change directory to the **qgroundcontrol.exe** location, and run it from there:

    ```sh
    cd "\Program Files (x86)\qgroundcontrol"
    qgroundcontrol --logging:full
    ```

  - When _QGroundControl_ starts you should see a separate console window open which will have the log output

- OSX

  - You must run _QGroundControl_ from Terminal. The Terminal app is located in Applications/Utilities. Once Terminal is open paste the following into it:

    ```sh
    cd /Applications/qgroundcontrol.app/Contents/MacOS/
    ./qgroundcontrol --logging:full
    ```

  - Log traces will output to the Terminal window.

- Linux

  ```sh
  ./qgroundcontrol-start.sh --logging:full
  ```

  - Log traces will output to the shell you are running from.
