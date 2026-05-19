# UI Overview

QGroundControl is organized into a small number of top-level views, each accessed from the toolbar on the left side of the screen.

## Toolbar

The toolbar is always visible and provides:

- **View switching** — click the **Q** icon in the toolbar to open the view selector, then choose Fly, Plan, or any other view.
- **View-specific content** — the rest of the toolbar changes based on the current view, showing relevant status and controls for that view.

## Main Views

### Fly View

The primary view while your vehicle is in the air. It shows a live map with the vehicle position, a heads-up display (HUD) with attitude and speed, and an instrument panel with telemetry values. Action buttons let you arm, take off, land, return to launch, and more.

See: [Fly View](../fly_view/fly_view.md)

### Plan View

Used before flight to create and upload autonomous missions. You can place waypoints on the map, configure survey patterns, set geofence boundaries, and define rally points. Missions are uploaded to the vehicle over the active link.

See: [Plan View](../plan_view/plan_view.md)

### Vehicle Configuration

Configure your vehicle's firmware, airframe, sensors, flight modes, and safety settings. This view walks you through each setup step with a sidebar checklist. Available options vary depending on the connected firmware (PX4 or ArduPilot).

See: [Vehicle Setup](../setup_view/firmware.md)

### Analyze View

Tools for post-flight analysis and debugging:

- **Log Download** — retrieve onboard flight logs.
- **GeoTag Images** — stamp survey photos with GPS coordinates.
- **MAVLink Console** — send commands directly to the autopilot.
- **MAVLink Inspector** — view raw MAVLink messages in real time.

See: [Analyze](../analyze_view/index.md)

### Application Settings

Persistent application preferences organized by category — general behavior, fly view layout, map providers, video streaming, RTK/NTRIP, telemetry logging, and more.
