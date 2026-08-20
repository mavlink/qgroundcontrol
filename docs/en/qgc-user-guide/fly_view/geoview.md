# GeoView (Tech Preview)

:::warning
GeoView is an experimental technology preview and is under active development. Expect missing features and rough edges.
It is too early to report issues against this feature — there is still too much work left to do.
:::

GeoView is a new map engine for QGroundControl that renders a single, seamless 2D/3D map: satellite/street imagery draped over real terrain elevation, with Google Earth-style camera controls. It displays your vehicles, the planned mission (with waypoint markers, connecting path, and drop lines showing each waypoint's height above the terrain), the flown flight path, the launch location, and the ground station position — all in true 3D.

:::info
GeoView is currently a display-only view. You must still use the regular [Fly View](fly_view.md) to control the vehicle (arm, takeoff, guided actions, mission control, and so on).
:::

The long-term goal is for the GeoView engine to eventually replace all map usage throughout QGroundControl, including the Fly and Plan views, providing one consistent 2D/3D map experience everywhere.

## Enabling GeoView

GeoView is disabled by default. To enable it, go to **Application Settings > GeoView** and turn on **Enable GeoView (preview)**. It then appears in the view selector dropdown in the main toolbar.

## View Controls

Camera gestures follow Google Earth semantics: the ground point under the cursor when a gesture starts stays pinned under the cursor throughout the gesture.

- **Mouse:**
  - **Pan**: Left-drag. The terrain point you grabbed follows the cursor.
  - **Orbit (rotate)**: Right-drag or middle-drag. The view rotates around the ground point you pressed on (marked by a pivot ring): left/right changes heading, up/down changes tilt between top-down and near-horizontal.
  - **Zoom**: Scroll wheel or trackpad scroll. Zooms toward/away from the point under the cursor.
- **Touch:**
  - **Pan**: Single-finger drag.
  - **Zoom**: Two-finger pinch, centered on the pinch point.
  - **Rotate**: Two-finger twist.

### Keyboard Support

Keyboard modifiers provide alternatives to the multi-button mouse gestures (useful with one-button mice and trackpads):

- **Shift + left-drag**: Orbit, same as right/middle-drag.
- **Ctrl + left-drag**: First-person look. The camera stays in place and the view rotates, like turning your head — drag toward where you want to look. On macOS both **Ctrl** and **Cmd** work.

### 2D/3D Modes

- The **3D/2D** button in the top-right switches between modes. Switching animates the camera tilt and flattens/raises the terrain; 2D mode behaves like a traditional top-down map (tilt is locked).
- The **compass** button rotates with the current heading; click it to animate the view back to north-up.

## Imagery and Terrain

GeoView drapes the same map imagery you have selected in **Application Settings > Maps** over its terrain surface. Terrain elevation comes from the Mapzen/Tilezen terrain tiles dataset (SRTM, 3DEP, GMTED2010, ETOPO1); attribution is shown in the lower-left overlay.
