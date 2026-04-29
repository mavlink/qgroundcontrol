# Log Setup (Analyze View)

The _Log Setup_ screen (**Analyze > Log Setup**) helps you configure common ArduPilot logging parameters and provides practical troubleshooting guidance.

## Common Parameters

The page exposes key parameters when they are available on the connected vehicle/firmware, including:

- `LOG_BACKEND_TYPE`
- `LOG_BITMASK`
- `LOG_DISARMED`
- `LOG_FILE_DSRMROT`
- `LOG_FILE_MB_FREE`
- `LOG_FILE_RATEMAX`
- `LOG_BLK_RATEMAX`
- `LOG_MAV_RATEMAX`
- `LOG_MAX_FILES`
- `LOG_REPLAY`
- `EK3_LOG_LEVEL`

All parameter edits are applied through the standard QGroundControl parameter system (Facts).

## Troubleshooting Checklist

- Use a known-good SD card and verify it is writable.
- Keep enough free space for expected log size.
- If logs show missing samples, lower block/logging rate limits.
- Enable disarmed/replay logging only when needed for debugging, because file size increases.
- Download and clear logs regularly on low-capacity onboard flash.
