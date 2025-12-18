# QGC Log Analyzer

Tools for analyzing QGroundControl logs and telemetry data.

## Installation

```bash
# For basic log analysis
# No dependencies required

# For MAVLink telemetry logs (.tlog)
pip install pymavlink
```

## Usage

### Analyze Application Logs

```bash
# Show all entries
./analyze_log.py ~/.local/share/QGroundControl/Logs/QGCConsole.log

# Show only errors
./analyze_log.py --errors QGCConsole.log

# Show errors and warnings
./analyze_log.py --warnings QGCConsole.log

# Filter by component
./analyze_log.py --component Vehicle QGCConsole.log

# Filter by message pattern
./analyze_log.py --message "parameter" QGCConsole.log

# Show statistics
./analyze_log.py --stats QGCConsole.log

# Show timeline
./analyze_log.py --timeline QGCConsole.log
```

### Analyze Telemetry Logs

```bash
# Analyze MAVLink telemetry log
./analyze_log.py flight.tlog

# Filter by MAVLink message type
./analyze_log.py --message "HEARTBEAT" flight.tlog

# Show statistics
./analyze_log.py --stats flight.tlog
```

## Log Locations

| Platform | Application Logs |
|----------|-----------------|
| Linux | `~/.local/share/QGroundControl/Logs/` |
| macOS | `~/Library/Application Support/QGroundControl/Logs/` |
| Windows | `%APPDATA%\QGroundControl\Logs\` |

Telemetry logs (`.tlog`) are saved in the same directory.

## Examples

### Find Connection Issues

```bash
./analyze_log.py --message "connect|disconnect|timeout" QGCConsole.log
```

### Find Parameter Errors

```bash
./analyze_log.py --component parameter --errors QGCConsole.log
```

### Analyze Flight Session

```bash
# Get overview
./analyze_log.py --stats flight.tlog

# Find GPS issues
./analyze_log.py --message "GPS|satellite" --warnings QGCConsole.log
```

## Output Formats

### Default Output

```
[12:34:56.789] [INFO ] [qgc.vehicle] Connected to vehicle 1
[12:34:57.123] [WARN ] [qgc.param] Parameter timeout
[12:34:58.456] [ERROR] [qgc.link] Connection lost
```

### Statistics Output

```
=====================================
LOG STATISTICS
=====================================

Total entries: 1234

By level:
  DEBUG: 500
  INFO: 600
  WARN: 100
  ERROR: 34

Top components:
  qgc.vehicle: 300
  qgc.mavlink: 250
  qgc.param: 150
```
