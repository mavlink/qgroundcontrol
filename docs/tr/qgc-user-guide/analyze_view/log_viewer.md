# Log Viewer (Analyze View)

The _Log Viewer_ screen (**Analyze > Log Viewer**) is a unified post-flight analysis tool that supports three log formats:

| Format              | Extension       | Firmware                                                                 |
| ------------------- | --------------- | ------------------------------------------------------------------------ |
| ArduPilot DataFlash | `.bin` / `.log` | ArduPilot (ArduCopter, ArduPlane, ArduRover, ArduSub) |
| PX4 ULog            | `.ulg`          | PX4                                                                      |
| MAVLink Telemetry   | `.tlog`         | Any                                                                      |

## Opening a Log

Use the **Open .bin**, **Open .ulg**, or **Open .tlog** button at the top of the screen to select a log file.
Click **Clear** to unload the current log.

The filename of the loaded log is shown in the toolbar to the right of the buttons.

## Firmware Log Analysis (.bin / .ulg)

When a DataFlash or ULog file is loaded the screen shows four tabs: **Charting**, **Map**, **Parameters**, and **Messages**.

### Charting Tab

The left panel shows summary information (field count, parameter count, event count, detected vehicle type) and a searchable list of all plottable fields.

- Toggle a field on or off using the slider next to its name.
- Use the **Search fields** box to filter by name.
- Click **Clear Selected** to remove all plotted fields at once.

The right panel shows a time-series chart of all selected fields.
Each field is plotted on its own auto-scaled Y axis.
Moving the cursor over the chart shows a popup with the current, minimum, and maximum value for each field, as well as the current flight mode.
Drag horizontally to zoom in; right-click to reset zoom.

### Map Tab

The Map tab displays the GPS flight path overlaid on a map, with the path auto-fitted to the screen on load.

For ArduPilot logs the path is derived from the `GPS` message (requires a valid 3D fix).
For PX4 ULog the path comes from `vehicle_global_position`.

An altitude mini-chart is shown below the map when GPS altitude data is available.
Click or drag on the altitude chart to move a position marker (yellow dot) along the flight path on the map.
Drag horizontally on the altitude chart to zoom in on a time range; right-click to reset.
The altitude popup shows the current, minimum, and maximum altitude for the full log.

The map cursor is linked to the charting tab: moving the cursor in either the main chart or the altitude chart updates the marker position in both places.

### Parameters Tab

Lists all parameters recorded in the log.
By default only parameters that differ from their system default are shown; toggle **Changed only** to see all parameters.
Use the search box to filter by name, value, or description.
Parameters that differ from their default are shown in bold with an orange indicator dot and the default value alongside.

### Messages Tab

Lists all status messages from the log (ArduPilot `MSG` messages or PX4 equivalent) in chronological order with timestamps.

## Telemetry Log Analysis (.tlog)

When a `.tlog` file is loaded the **Charting** tab shows a playback control bar in the left panel.

| Control          | Function                                                         |
| ---------------- | ---------------------------------------------------------------- |
| **Play / Pause** | Start or pause log replay                                        |
| Speed selector   | Set replay speed (0.1× – 10×) |
| Seek slider      | Jump to any point in the log                                     |
| Time display     | Shows current and total log duration                             |

While replaying, all live vehicle data views (HUD, instruments, MAVLink Inspector) update as if the vehicle were connected.
The right panel of the Charting tab shows a live MAVLink Inspector for the replaying vehicle.

## Typical Workflow

1. Download logs from **Analyze > Log Download**.
2. Open the downloaded `.bin`, `.ulg`, or `.tlog` in **Analyze > Log Viewer**.
3. For firmware logs: select fields of interest in the left panel and inspect the chart, map, parameters, and messages.
4. For telemetry logs: use the playback controls to step through the flight.
