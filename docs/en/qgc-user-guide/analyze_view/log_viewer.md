# Log Viewer (Analyze View)

The _Log Viewer_ screen (**Analyze > Log Viewer**) provides a unified workflow for post-flight analysis:

- Open ArduPilot DataFlash logs (`.bin`/`.log`) for parameter and event inspection.
- Open telemetry logs (`.tlog`) and replay them with timeline controls.
- Reuse existing MAVLink inspection and charting tools while replaying telemetry.

## Typical Workflow

1. Download logs from **Analyze > Onboard Logs**.
1. Open the downloaded `.bin` or `.tlog` file in **Analyze > Log Viewer**.
1. For `.tlog`, use playback controls (play/pause, speed, seek) to inspect behavior over time.
1. For `.bin`, review extracted parameters and events, then select signals for plotting.

## Performance Notes

- Very large logs can take noticeable time to parse.
- Prefer loading logs from local SSD storage for faster indexing.
- If onboard storage is slow and logs contain gaps, reduce logging rate using related parameters in **Analyze > Log Setup**.
