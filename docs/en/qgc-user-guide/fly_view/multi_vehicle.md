# Multiple Vehicles

QGroundControl can connect to several vehicles at the same time and lets you supervise all of them while
commanding one — or send a small set of actions to several vehicles at once. This page explains the two
different targeting concepts and what applies to which.

::: warning
Each connected vehicle must have a **unique MAVLink system id**, otherwise QGC treats them as the same vehicle.
The classic symptom is the map/Plan view jerking around between positions. On PX4 firmwares the id is the
`MAV_SYS_ID` parameter; on ArduPilot it is `SYSID_THISMAV`. Change the id on the vehicle *before* connecting
them together. See also [Toolbar & Indicators](fly_view_toolbar.md#multi-vehicle-selector).
:::

## Active vehicle vs. selected vehicles

QGC has two distinct targeting concepts when several vehicles are connected:

- The **active vehicle** is the single vehicle the normal user interface is talking about and to: the toolbar
  status and indicators, the instrument panel, the pre-flight checklist, and **all regular fly tools and guided
  actions** (Takeoff, Land, Return, Pause, Go To Location, orbit, altitude changes, …) apply to the active
  vehicle only.
- The **selected vehicles** are a separate set, chosen in the vehicle list, that only the four
  [multi-vehicle actions](#multi-vehicle-actions) operate on. Selecting vehicles does not change which vehicle
  the rest of the UI is showing or commanding.

## Switching the active vehicle

- The **Multi-Vehicle Selector** appears in the toolbar when more than one vehicle is connected and switches the
  active vehicle from anywhere.
- In the **vehicle list panel** (top-right panel in the Fly view), the active vehicle's row is highlighted.

Before sending any command, confirm the vehicle named in the toolbar is the aircraft you intend to command.

## The vehicle list

The vehicle list page of the top-right panel shows one row per connected vehicle with its basic state. Rows can
be individually selected or deselected for multi-vehicle actions, and **Select All** / **Deselect All** buttons
act on the whole fleet.

## Multi-vehicle actions

Four actions can be sent to *all currently selected vehicles*:

| Action | Effect on each selected vehicle |
| --- | --- |
| **Arm** | Arms the vehicle |
| **Disarm** | Disarms the vehicle |
| **Start** | Starts the uploaded mission (vehicles that are not armed are skipped) |
| **Pause** | Pauses at the current position |

Each action shows a confirmation slider before anything is sent. Every other action in the interface remains
single-vehicle and applies to the active vehicle only.

## Things to be aware of

- **Pre-flight status is per-vehicle.** The toolbar readiness/status display, sensor status and checklist refer
  to the *active* vehicle only — switch through each vehicle to review the others before a multi-vehicle
  operation.
- **Missions are per-vehicle.** Each vehicle flies its own uploaded mission; **Start** does not synchronise or
  deconflict missions between vehicles.
- **The active vehicle can change on link loss.** If the active vehicle's connection drops, QGC may make another
  connected vehicle active. Re-check which vehicle is active before commanding after any connection event.
