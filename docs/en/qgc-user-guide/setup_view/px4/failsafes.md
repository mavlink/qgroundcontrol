# Failsafes (PX4)

The Failsafes page configures what the vehicle does when it loses a link, runs low on power or detects a fault.
It brings the failsafe settings that PX4 spreads across several parameter groups into one place.
It is designed for PX4 1.17 or later. Older firmware shows only the settings it has.

The page only shows settings the connected vehicle has, and only the failsafes that apply to its type.
For example, _Quadchute_ is shown for VTOLs only, and _GPS failure loiter_ for fixed wings and VTOLs.
The choices, units and limits of each setting come from the vehicle's parameter metadata.

## Failsafe Rows

Each row shows:

- **Failsafe** — the name, the parameters it uses and a short description.
  The dot next to the name is green when the failsafe is active.
  It is hollow when the failsafe is turned off, for example a link loss action set to _Disabled_.
  A _Warning only_ chip marks failsafes that only show a message and never change what the vehicle is doing.
- **Cells** — one captioned cell per setting: the trigger (a timeout or threshold), the action, and any options.
  A colored marker next to an action shows how severe it is: warning, hold, mode fallback, return, land, or terminate/disarm.
  Cells marked _Reboot required_ only take effect after the vehicle reboots.

Some failsafes have an action that PX4 does not let you change, such as the _Maximum flight time_ failsafe which always returns.
These show the action as fixed text.

Rows are grouped by cause:

- **Failsafe Behavior** — how long the vehicle holds before acting, and the stick override that lets the pilot take back control.
- **Link Loss** — manual control (RC or joystick), data link (GCS), offboard and onboard computer loss.
- **Power** — battery levels and action, remaining flight time, and maximum flight time.
- **Navigation & Environment** — geofence, low position accuracy, position estimate loss, GNSS loss, high wind and traffic avoidance.
- **Vehicle Health** — attitude failure, motor failure, ESC arming failure, imbalanced propeller, altitude loss, parachute and external trigger.
- **VTOL** — quadchute.
- **Return Mode** — where the vehicle returns to and what it does there. Every _Return_ action above uses these settings.

Select **Active** to hide the failsafes that are turned off.
Changes are written to the vehicle immediately and can be made in flight.

PX4 flight termination is disabled by default and is not configured here.
Until it is enabled, an attitude failure only warns the pilot once the vehicle is past its takeoff lockdown window.
