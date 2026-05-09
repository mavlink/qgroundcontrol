# Logging (ArduPilot)

The _Logging_ setup page (**Vehicle Config > Logging**) is available for ArduPilot vehicles and allows you to configure how and what the flight controller logs.

## Parameters

All parameters are optional — controls for parameters not supported by the connected firmware are hidden automatically.

### Storage

| Parameter | Description |
|---|---|
| `LOG_BACKEND_TYPE` | Which backends are active: SD card file, MAVLink stream, and/or onboard flash (bitmask). |
| `LOG_BITMASK` | Which data groups are included in the log (bitmask). |
| `LOG_MAX_FILES` | Maximum number of log files retained before the oldest is rotated out. |
| `LOG_FILE_MB_FREE` | Minimum free SD card space (MB) that must remain available before logging begins. |

### Rate Limits

| Parameter | Description |
|---|---|
| `LOG_FILE_RATEMAX` | Maximum file logging rate (Hz). Lower this if the SD card cannot keep up. |
| `LOG_BLK_RATEMAX` | Maximum block (onboard flash) logging rate (Hz). |
| `LOG_MAV_RATEMAX` | Maximum MAVLink log stream rate (Hz). |

### Options

| Parameter | Description |
|---|---|
| `LOG_DISARMED` | Enable logging while the vehicle is disarmed (useful for pre-arm diagnostics). |
| `LOG_FILE_DSRMROT` | Rotate the log file after each disarm/rearm sequence. |
| `LOG_REPLAY` | Log extra data needed for EKF replay analysis. |
| `EK3_LOG_LEVEL` | Controls EKF3 logging verbosity (0 = full, 3 = disabled). |
