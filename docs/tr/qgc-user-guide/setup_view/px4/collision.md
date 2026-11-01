# Collision (PX4)

The Collision page configures collision prevention, which keeps a multicopter away from obstacles seen by distance sensors or a companion computer.
Collision prevention only works in Position mode.
It is designed for PX4 1.17 or later.

The page only appears when the connected vehicle has the collision prevention settings.
The choices, units and limits of each setting come from the vehicle's parameter metadata.
Changes are written to the vehicle immediately and can be made in flight.

## Collision Prevention

- **Prevention** — the minimum distance the vehicle keeps to obstacles (_0_ or negative turns collision prevention off), and whether it may move in directions where no sensor is looking.
- **Tuning** — the combined sensor and control delay, and how far the vehicle may steer left or right of the commanded direction. The defaults suit most vehicles.

## Display

**Show obstacle distance overlay** draws the distances measured around the vehicle on the fly view.
