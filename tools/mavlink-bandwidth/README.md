# MAVLink bandwidth test endpoint

`APM/scripts/mavlink_bandwidth.lua` is the ArduPilot endpoint for QGroundControl's **Analyze > MAVLink
Bandwidth** tool. It uses the standard MAVLink `TUNNEL` message with the default unknown payload type and a
magic/versioned inner protocol; no ArduPilot source change or custom MAVLink dialect is required.

![QGC MAVLink bandwidth test](mav-iperf.png)

## Install

1. Enable ArduPilot scripting for a supported board.
2. Upload `APM/scripts/mavlink_bandwidth.lua` to the vehicle's `APM/scripts` directory.
3. Restart scripting or reboot the vehicle while disarmed.
4. Open **Analyze > MAVLink Bandwidth** and select **Probe Endpoint**.

The test is disarmed-only and is capped at 2 Mbit/s of test payload for at most 60 seconds.

## Measurement boundary

The result is end-to-end available MAVLink bandwidth. It includes QGC, the selected link, ArduPilot MAVLink queues,
and the Lua scheduler. It is not a measurement of raw RF bitrate. QGC reports test-payload goodput separately from
all bytes observed on the selected link.

ArduPilot's scripting MAVLink receive queue and message registrations are process-global. Scripts that also call
`mavlink:init()` can replace this endpoint's registrations; consolidate MAVLink registration when combining this
endpoint with other MAVLink-consuming scripts.

For SITL, add `sitl.params` to the normal vehicle defaults and place the Lua file in the runtime directory's `scripts`
subdirectory.
