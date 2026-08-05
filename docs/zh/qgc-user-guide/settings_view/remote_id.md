# Remote ID

Configure drone Remote ID broadcast settings to comply with FAA or EU regulations.

## Region

- **Region of operation** — FAA or EU (controls which fields are shown)

## Basic ID

- **Broadcast** — enable Basic ID broadcasting
- **Basic ID Type** — None / Serial Number / CAA Registration / UTM (USS) Assigned / Specific Session
- **UA type** — aircraft type classification (Helicopter/Multirotor, Airplane, Gyroplane, VTOL, Airship, Glider, etc.)
- **Basic ID** — the identification string (max 20 characters)

## Operator ID

- **Broadcast** — enable Operator ID broadcasting (FAA only)
- **Operator ID type** — CAA
- **Operator ID** — your operator registration number (max 20 characters)

## Self ID

- **Broadcast** — enable Self ID broadcasting
- **Broadcast Message** — Flight Purpose / Emergency / Extended Status
- **Flight Purpose** / **Extended Status** / **Emergency Text** — message strings (max 23 characters)

## Ground Station Location

- **Location Type** — Takeoff (auto) / Live GNSS / Fixed coordinates
- **Latitude/Longitude/Altitude** — manual coordinates when using Fixed type

## GCS Position

Read-only display of current GCS latitude, longitude, and HDOP.

## GPS Location

Configure an external NMEA GPS device for GCS position (device, baudrate, port selection).

## EU Vehicle Info (EU region only)

- **Classification Type** — Undeclared / EU
- **Category** — Undeclared / Open / Specific / Certified
- **Class** — EU drone class designation
