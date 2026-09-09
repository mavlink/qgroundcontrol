# GPS diagnostics acceptance

The automated GPS tests cover source policy, session isolation, transport replay,
configuration evidence, and QML controls. Run the focused tests with:

```sh
ctest --test-dir build --build-config Debug --output-on-failure -R 'GPS|NMEA|RTK|NTRIP|RTCM|PositionManager'
```

## Position source selection

In **Application Settings → GPS → Ground-Station Position Source**, connection
priority preserves the existing local receiver, NMEA, then device preference.
The local receiver is eligible only when **Use receiver for ground-station
position** is enabled. A pinned source stays selected when its fix is lost.
Automatic selection enables fallback to a healthy source and waits five seconds
of continuous health before returning to a recovered preferred source.

With two position sources available:

1. Select each pinned source and confirm the displayed source and status.
2. Stop its input. Confirm the displayed position becomes unavailable instead of
   changing to the other source.
3. Select automatic mode, restore both inputs, then stop the preferred input.
   Confirm fallback occurs after the preferred fix becomes stale.
4. Restore the preferred input and confirm selection returns only after the
   recovery delay. Repeated brief recoveries must not cause repeated switching.

## Receiver observations

Select **Local receiver** or **NMEA** in Receiver Observations. Confirm that
satellite identities, used status, signal strength, and angles follow that input.
Unavailable metadata should remain unknown. Stop the input and confirm the
presentation expires; reconnecting must not retain the previous session's data.
Dual-antenna baseline and heading values require a receiver that reports them.

## Configuration evidence

Connect a supported receiver after changing a receiver setting. Expand Receiver
Configuration Status and compare requested, acknowledged, and reported values.
Acknowledgment is command acceptance; reported values come from a separate query.
Readback is optional and currently limited to supported modern UBX native output.
Missing readback must not prevent an otherwise usable connection. A reported
mismatch must remain distinguishable from a rejected request.

## Capture and replay

Start recording before connecting to capture receiver initialization. Stop the
recording after reproducing the issue, then export it to a JSON file. Recording
also supports starting on an existing connection, but such a capture lacks the
initial configuration exchange. Starting again replaces the in-memory capture.

Recordings contain receiver bytes and may contain precise locations. Metadata
excludes endpoint names and credentials; NTRIP sessions are not recorded.
The capture stops at its bounded event or memory limit. Exported files retain
stream identities when native and NMEA inputs are active together.
See [Replay/README.md](Replay/README.md) for loading captures and selecting a
stream. Hardware acceptance remains separate from synthetic replay results.
