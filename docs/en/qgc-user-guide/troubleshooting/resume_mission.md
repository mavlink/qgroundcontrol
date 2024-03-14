# Resume Mission Failures

The process of resuming a mission after a battery swap is a fairly complex process within QGC.

The two main areas that are most problematic are:

- The _Resume Mission_ dialog doesn't display when it should and you are just left with a Start Mission slider.
- The new mission generated from _Resume Mission_ is not quite correct with respect to recreation of waypoints and/or camera commands.

::: warning
In order for the _QGroundControl_ development team to debug these issues the following information **must be supplied** in any github issue entered against _Resume Mission_.
:::

## Common Steps for Resume Mission Dialog/Generation {#common_steps}

The following steps are required for debugging both types of problems:

1. Restart QGC
1. Turn on [console logging](../settings_view/console_logging.md) with the log option: `GuidedActionsControllerLog`.
1. Enable [telemetry logging](../settings_view/general.md#miscellaneous) (**Settings > General**).
1. Start the mission.
1. Fly till you need a battery swap.

   ::: tip
   Alternatively you can attempt to reproduce the problem by manually RTL from the middle of the middle of the mission (though this may not always reproduce the problem).
   :::

1. Once the vehicle lands and disarms you should get the _Resume Mission_ dialog.

   ::: info
   If not there is a possible bug in QGC.
   :::

### Resume Mission Dialog Problems

For _Resume Mission Dialog_ problems follow the [common steps above](#common_steps), and then:

7. Save the _Console Log_ to a file.
1. Place the _Console Log_, _Telemetry Log_ and _Plan File_ someplace which you can link to in the issue.
1. Create the issue with details and links to all three files.

## Resume Mission Generation Problems

For _Resume Mission Generation_ problems follow the [common steps above](#common_steps), and then:

7. Click **Resume Mission**.
1. The new mission should be generated.
1. Go to [Plan View](../plan_view/plan_view.md).
1. Select **Download** from the _File/Sync_ menu.
1. Save the _Modified Plan_ to a file.
1. Save the _Console Log_ to a file.
1. Place the _Console Log_, _Telemetry Log_, _Original Plan_ file and _Modified Plan_ file someplace which you can link to in the issue.
1. Create the issue with details and links to all four files.
