# MAVLink bandwidth test

QGroundControl's **Analyze > MAVLink Bandwidth** tool has two test modes:

- **MAVFTP** works with stock PX4 and ArduPilot. QGC generates a temporary file, measures upload followed by
  download, verifies the data using SHA-256, and removes the remote file.
- **Streaming** uses the included ArduPilot Lua endpoint for controlled packet-rate and loss measurements. It uses
  the standard MAVLink `TUNNEL` message with a magic/versioned inner protocol.

Streaming mode:

![QGC MAVLink streaming bandwidth test](mav-iperf.png)

MAVFTP mode:

![QGC MAVFTP bandwidth test](mavftp.png)

## MAVFTP mode

MAVFTP mode requires no vehicle-side installation. Select the file size, then start the test. QGC measures the upload
and then downloads the same file, reporting separate goodput and selected-link rates for each direction. The download
also verifies the uploaded data before cleanup.

PX4 test files use `/fs/microsd`; ArduPilot test files use `/APM`. Tests are disarmed-only, and remote files are
named with a random token to avoid replacing existing data.

## ArduPilot streaming mode

1. Enable ArduPilot scripting for a supported board.
2. In **Analyze > MAVLink Bandwidth**, select **Install Lua Script and Reboot** to upload QGC's embedded copy to
   `APM/scripts/mavlink_bandwidth.lua` and reboot the vehicle while disarmed.
3. After the vehicle reconnects, select **Probe Endpoint**.

Streaming tests are capped at 2 Mbit/s of test payload for at most 60 seconds.

## Measurement boundary

The result is end-to-end available MAVLink bandwidth, not raw RF bitrate. MAVFTP results include reliable-transfer
protocol and storage overhead. Streaming results include QGC, the selected link, ArduPilot MAVLink queues, and the
Lua scheduler. QGC reports test-payload goodput separately from all bytes observed on the selected link.

ArduPilot's scripting MAVLink receive queue and message registrations are process-global. Scripts that also call
`mavlink:init()` can replace this endpoint's registrations; consolidate MAVLink registration when combining this
endpoint with other MAVLink-consuming scripts.

For SITL, add `sitl.params` to the normal vehicle defaults and place the Lua file in the runtime directory's `scripts`
subdirectory.
