# Return & Land (PX4)

The Return & Land page configures how the vehicle returns, what it does when it arrives, and how it lands.
Every _Return_ action uses these settings, including the failsafes on the [Failsafes](failsafes.md) page and the Return flight mode.
It is designed for PX4 1.17 or later. Older firmware shows only the settings it has.

The page only shows settings the connected vehicle has, and only those that apply to its type.
The choices, units and limits of each setting come from the vehicle's parameter metadata.
Changes are written to the vehicle immediately and can be made in flight.

## Return

- **Route** — where the vehicle goes and how high it flies: the destination, the altitude it climbs to, and the altitude cone and approach settings.
- **Arrival** — what the vehicle does once it reaches the destination: the loiter altitude, how long it loiters before landing (_-1_ loiters and does not land), and the loiter radius for fixed wings and VTOLs.

## Landing

- **Descent** — how fast the vehicle descends close to the ground, for multicopters and VTOLs.
  _Slow descent below_ must be higher than _Landing speed below_.
  The _Landing descent rate_ is set by PX4 itself while the overall vertical velocity limit on the Flight Behavior page is on, so it can't be edited then.
- **Touchdown** — precision landing, and how long after landing the vehicle disarms (_0_ or negative disables).
