# Resume Mission Failures

The process of resuming a mission after a battery swap is a fairly complex process within QGC.

The two main areas that are most problematic are:

* The *Resume Mission* dialog doesn't display when it should and you are just left with a Start Mission slider.
* The new mission generated from *Resume Mission* is not quite correct with respect to recreation of waypoints and/or camera commands.

> **Warning** In order for the *QGroundControl* development team to debug these issues the following information **must be supplied** in any github issue entered against *Resume Mission*.

## Common Steps for Resume Mission Dialog/Generation {#common_steps}

The following steps are required for debugging both types of problems:

1. Restart QGC
2. Turn on [console logging](../SettingsView/console_logging.md) with the log option: `GuidedActionsControllerLog`.
3. Enable [telemetry logging](../SettingsView/General.md#miscellaneous) (**Settings > General**).
4. Start the mission.
5. Fly till you need a battery swap. > **Tip** Alternatively you can attempt to reproduce the problem by manually RTL from the middle of the middle of the mission (though this may not always reproduce the problem).
6. Once the vehicle lands and disarms you should get the *Resume Mission* dialog. > **Note** If not there is a possible bug in QGC.

### Resume Mission Dialog Problems

For *Resume Mission Dialog* problems follow the [common steps above](#common_steps), and then:

7. Save the *Console Log* to a file.
8. Place the *Console Log*, *Telemetry Log* and *Plan File* someplace which you can link to in the issue.
9. Create the issue with details and links to all three files.

## Resume Mission Generation Problems

For *Resume Mission Generation* problems follow the [common steps above](#common_steps), and then:

7. Click **Resume Mission**.
8. The new mission should be generated.
9. Go to [Plan View](../PlanView/PlanView.md).
10. Select **Download** from the *File/Sync* menu.
11. Save the *Modified Plan* to a file.
12. Save the *Console Log* to a file.
13. Place the *Console Log*, *Telemetry Log*, *Original Plan* file and *Modified Plan* file someplace which you can link to in the issue.
14. Create the issue with details and links to all four files.