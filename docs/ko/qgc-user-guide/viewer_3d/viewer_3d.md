# 3D View

:::warning
The entire 3D View feature is a work in progress. You may hit various issues while using it.
:::

:::info
The 3D View is not available on Android, as rendering is not yet reliable across Android GPUs and drivers.
:::

The 3D View is used to visualize and monitor the vehicle, the environment, and the planned mission in 3D. Most of the capabilities available in the [Fly View](../fly_view/fly_view.md)  is also available in the 3D View.

You can use it to:

- To import and display the 3D map for any region of interest downloaded from the OpenStreetMap website (.osm file).
- Display the vehicle along with its mission in 3D.
- And most of the capabilities of the [Fly View](../fly_view/fly_view.md), including:
  - Run an automated [pre-flight checklist](../fly_view/hud.md#preflight_checklist).
  - Arm the vehicle (or check why it won't arm).
  - Control missions: [start](../fly_view/hud.md#start_mission), [continue](../fly_view/hud.md#continue_mission), [pause](../fly_view/hud.md#pause), and [resume](../fly_view/hud.md#resume_mission).
  - Guide the vehicle to [arm](../fly_view/hud.md#arm)/[disarm](../fly_view/hud.md#disarm)/[emergency stop](../fly_view/hud.md#emergency_stop), [takeoff](../fly_view/hud.md#takeoff)/[land](../fly_view/hud.md#land), [change altitude](../fly_view/hud.md#change_altitude), and [return/RTL](../fly_view/hud.md#rtl).
  - Switch between a map view and a video view (if available)
  - Display video, mission, telemetry, and other information for the current vehicle, and also switch between connected vehicles.

# UI Overview

The main elements of the 3D View are the same as the [Fly View](../fly_view/fly_view.md), with an added 3D environment.

**Enabling the 3D View:** The 3D View is disabled by default. To enable it, go to **Settings > Fly View**, and under the **3D View** settings group, toggle the **Enabled** switch.

To open the 3D View, when you are in the [Fly View](../fly_view/fly_view.md), select the 3D View icon from the toolbar on the left.

## View Controls

The camera controls work like Gazebo: gestures are anchored to the ground point under the cursor, so the spot you grab stays under the cursor while you pan, orbit, or zoom.

- **Mouse:**
  - **Pan**: Press and hold the left mouse button, then move the cursor. The ground point you grabbed follows the cursor.
  - **Orbit (rotate)**: Press and hold the middle mouse button (scroll wheel), then move the cursor. The view rotates around the ground point under the cursor: dragging left/right changes heading (a full window width of drag is one full revolution), dragging up/down changes the viewing angle between top-down and near-horizontal.
  - **Orbit (trackpad-friendly)**: Hold **Shift** and drag with the left button. This is useful on trackpads without a middle button.
  - **Zoom**: Use the mouse wheel or trackpad scroll, or press and hold the right mouse button and drag up (zoom in) or down (zoom out). Zooming moves toward or away from the ground point under the cursor, keeping it fixed on screen.

- **Touchscreen:**
  - **Pan**: Tap and move with a single finger.
  - **Zoom**: Pinch with two fingers, moving them together or apart.
  - **Rotate**: Twist with two fingers. The view rotates around the ground point under the gesture, following your fingers.
  - **Tilt**: Swipe up or down with two fingers held together. Swiping up tilts toward the horizon, swiping down toward a top-down view.

A **scale bar** is shown in the same position as in the Fly View (top of the view, next to the tool strip) and shows the ground distance covered on screen. In a perspective view the scale varies across the screen; the value shown is exact for the ground plane through the screen center.

To visualize the 3D map of a particular area in the 3D viewer, you have to download the .osm file of that area from the [OpenStreetMap](https://www.openstreetmap.org/#map=16/47.3964/8.5498) website and then import it through the **3D View** settings. More details on the **3D View** settings can be found in the next section.

If no OSM file is loaded, buildings are not shown, but the 3D View still displays the vehicle over a map-tile ground patch centered on the vehicle's home position, viewed from the side by default.

# Settings

You can change the settings of the 3D View from **Application Settings** ->**Fly View** tab under the **3D View** settings group.
The following properties can be modified in the 3D View settings group:

- **Enabled**: To enable or disable the 3D View.
- **3D Map File**: The path to the .osm file of a region of interest to be visualized in the QGC. The .osm file can be uploaded by clicking on the **Select File** button. To clear the 3D View from the previously loaded .osm file, you can click on the **Clear** button.
- **Average Building Level Height**: This parameter determines the height of each storey of the buildings, as in .osm file sometimes the height of the buildings is specified in terms of the level/storey.
- **Vehicle Altitude Bias**: This refers to the bias in the altitude of vehicles and their missions with respect to the ground level. It is helpful in cases where the estimated altitude of the vehicle by its flight control is biased, as the relative altitude is currently used in the 3D View.
