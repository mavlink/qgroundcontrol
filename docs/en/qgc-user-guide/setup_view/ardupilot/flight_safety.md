# Flight Safety (ArduPilot)

The Flight Safety page configures Return to Launch behavior, geofence settings, and arming checks.

## Return to Launch

Settings vary by vehicle type:

- **RTL altitude** — choose between returning at current altitude or a specified altitude (`RTL_ALT`)
- **Loiter above home** — optionally loiter at the return point before landing, with configurable loiter time (`RTL_LOIT_TIME`)
- **Final landing altitude** — altitude for the final descent phase (`RTL_ALT_FINAL`)
- **Landing speed** — descent rate during final landing (`LAND_SPD`)
- **RTL radius** — loiter radius at the return point (Plane, `RTL_RADIUS`; 0 uses the default waypoint loiter radius)
- **Auto-land** — automatically land after returning (Plane, `RTL_AUTOLAND`)

## GeoFence

- **Enable fence** — turn the geofence on or off (`FENCE_ENABLE`)
- **Fence type** — bitmask selecting which limits to enforce: max altitude, min altitude, circle radius, polygon boundaries (`FENCE_TYPE`)
- **Max altitude** / **Min altitude** — altitude limits in meters
- **Circle radius** — cylindrical fence radius around home (`FENCE_RADIUS`)
- **Fence margin** — buffer distance before the fence boundary triggers an action (`FENCE_MARGIN`)
- **Fence action** — what to do when the fence is breached (RTL / Land / Report Only / etc.)
- **Auto-enable** — automatically enable the fence on arming or mode change (`FENCE_AUTOENABLE`)
- **Return altitude** (Plane) — altitude for the fence return path (`FENCE_RET_ALT`)
- **Return to nearest rally point** (Plane) — use the closest rally point instead of home (`FENCE_RET_RALLY`)

## Arming Checks

::: warning
Disabling arming checks can be dangerous. Only disable checks if you understand the safety implications.
:::

- **Arming checks** — bitmask of pre-arm checks to enforce (`ARMING_CHECK`). The first option enables/disables all checks.
- **Skip checks** — alternative bitmask to skip specific checks (`ARMING_SKIPCHK`)
