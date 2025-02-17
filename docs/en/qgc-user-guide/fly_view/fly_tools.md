# Fly Tools

## Pre Flight Checklist {#preflight_checklist}

An automated preflight checklist can be used to run through standard checks that the vehicle is configured correctly and it is safe to fly.

To view the checklist, first enable the tool by navigating to [Application Settings > General > Fly View](../settings_view/general.md) and selecting the **Use preflight checklist** checkbox.
The tool will then be added to the _Flight Tools_.
Press it to open the checklist:

![Pre Flight Checklist](../../../assets/fly/pre_flight_checklist.jpg)

Once you have performed each test, select it on the UI to mark it as complete.

## Takeoff {#takeoff}

:::tip
If you are starting a mission for a multicopter, _QGroundControl_ will automatically perform the takeoff step.
:::

To takeoff (when landed):

1. Press the **Takeoff** button in the _Fly Tools_ (this will toggle to a **Land** button after taking off).
1. Optionally set the takeoff altitude in the right-side vertical slider.
  - You can slide up/down to change the altitude
  - You can also click on the specified altitude (10 ft in example) and then type in a specific altitude.
1. Confirm takeoff using the slider.

![takeoff](../../../assets/fly/takeoff.png)

## Land {#land}

You can land at the current position at any time while flying:

1. Press the **Land** button in the _Fly Tools_ (this will toggle to a **Takeoff** button when landed).
1. Confirm landing using the slider.

## RTL/Return

Return to a "safe point" at any time while flying:

1. Press the **RTL** button in the _Fly Tools_.
1. Confirm RTL using the slider.

::: info
Vehicles commonly return to the "home" (takeoff) location and land.
This behaviour depends on the vehicle type and configuration.
For example, rally points or mission landings may be used as alternative return targets.
:::

## Change Altitude {#change_altitude}

You can change altitude while flying, except when in a mission:

1. Press the **Actions** button on the _Fly Tools_
1. Select the _Change Altitude_ button
2. Select the new altitude from the vertical slider
3. Confirm the action