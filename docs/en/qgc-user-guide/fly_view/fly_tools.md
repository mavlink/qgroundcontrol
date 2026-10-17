# Fly Tools

## Action Confirmation {#confirmation}

Safety-critical actions (takeoff, land, RTL, etc.) require confirmation before they are executed. When you trigger one of these actions, a confirmation button appears with chevrons on its left side. To confirm, click and hold the button until the action is accepted. This prevents accidental activation of critical commands.

## Pre Flight Checklist {#preflight_checklist}

An automated preflight checklist can be used to run through standard checks that the vehicle is configured correctly and it is safe to fly.

To view the checklist, first enable it by navigating to [Application Settings > General > Fly View](../settings_view/general.md) and selecting the **Use preflight checklist** checkbox.
The checklist is then available on the **Pre-Flight Checklist** tab of the main status dropdown (click the status text at the left of the toolbar).
The dropdown opens on this tab until the checklist has passed.

Once you have performed each test, select it on the UI to mark it as complete.

All checklist sections are shown at once. A section is locked (marked **(locked)**) until every section above it has passed, but its items still show their current state, so problems detected automatically — such as muted audio or too few GPS satellites — are visible from the start.
Items are colored by state: red for a failed check that must be fixed, orange for a check that still needs confirming, and green for a passed check.
The tab shows **Passed** once every check has passed, otherwise **In Progress**.

## Takeoff {#takeoff}

:::tip
If you are starting a mission for a multicopter, _QGroundControl_ will automatically perform the takeoff step.
:::

To takeoff (when landed):

1. Press the **Takeoff** button in the _Fly Tools_ (this will toggle to a **Land** button after taking off).
1. Optionally set the takeoff altitude in the right-side vertical slider.

- You can slide up/down to change the altitude
- You can also click on the specified altitude (10 ft in example) and then type in a specific altitude.

1. [Confirm](#confirmation) the action.

## Land {#land}

You can land at the current position at any time while flying:

1. Press the **Land** button in the _Fly Tools_ (this will toggle to a **Takeoff** button when landed).
1. [Confirm](#confirmation) the action.

## RTL/Return

Return to a "safe point" at any time while flying:

1. Press the **RTL** button in the _Fly Tools_.
1. [Confirm](#confirmation) the action.

::: info
Vehicles commonly return to the "home" (takeoff) location and land.
This behaviour depends on the vehicle type and configuration.
For example, rally points or mission landings may be used as alternative return targets.
:::

## Change Altitude {#change_altitude}

You can change altitude while flying, except when in a mission:

1. Press the **Actions** button on the _Fly Tools_
1. Select the _Change Altitude_ button
1. Select the new altitude from the vertical slider
1. Confirm the action
