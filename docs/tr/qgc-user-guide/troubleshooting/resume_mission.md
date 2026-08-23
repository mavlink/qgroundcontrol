# Resume Mission Failures

The process of resuming a mission after a battery swap is a fairly complex process within QGC.

The two main areas that are most problematic are:

- The _Resume Mission_ dialog doesn't display when it should and you are just left with a Start Mission slider.
- The new mission generated from _Resume Mission_ is not quite correct with respect to recreation of waypoints and/or camera commands.

:::warning
In order for the _QGroundControl_ development team to debug these issues the following information **must be supplied** in any github issue entered against _Resume Mission_.
:::

## Common Steps for Resume Mission Dialog/Generation {#common_steps}

The following steps are required for debugging both types of problems:

1. Restart QGC

2. Turn on [console logging](../troubleshooting/console_logging.md) with the logging category: `QMLControls.GuidedActionsController`.

3. Enable [telemetry logging](../settings_view/general.md#miscellaneous) (**Settings > General**).

4. Start the mission.

5. Fly till you need a battery swap.

   ::: tip
   Alternatively you can attempt to reproduce the problem by manually RTL from the middle of the middle of the mission (though this may not always reproduce the problem).
   :::

6. Once the vehicle lands and disarms you should get the _Resume Mission_ dialog.

   ::: info
   If not there is a possible bug in QGC.
   :::

### Resume Mission Dialog Problems

For _Resume Mission Dialog_ problems follow the [common steps above](#common_steps), and then:

1. Save the _App Log_ to a file.
2. Place the _App Log_, _Telemetry Log_ and _Plan File_ someplace which you can link to in the issue.
3. Create the issue with details and links to all three files.

## Resume Mission Generation Problems

For _Resume Mission Generation_ problems follow the [common steps above](#common_steps), and then:

1. Click **Resume Mission**.
2. The new mission should be generated.
3. Go to [Plan View](../plan_view/plan_view.md).
4. Select **Download** from the _File/Sync_ menu.
5. Save the _Modified Plan_ to a file.
6. Save the _App Log_ to a file.
7. Place the _App Log_, _Telemetry Log_, _Original Plan_ file and _Modified Plan_ file someplace which you can link to in the issue.
8. Create the issue with details and links to all four files.
