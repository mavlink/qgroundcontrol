# Parameters File Format

```
# Onboard parameters for Vehicle 1
#
# # Vehicle-Id Component-Id Name Value Type
1	1	ACRO_LOCKING	0	2
1	1	ACRO_PITCH_RATE	180	4
1	1	ACRO_ROLL_RATE	180	4
1	1	ADSB_ENABLE	0	2
```

Above is an example of a parameter file with four parameters. The file can include as many parameters as needed.

Comments are preceded with a `#`.

This header: `# MAV ID  COMPONENT ID  PARAM NAME  VALUE` describes the format for each row:

* `Vehicle-Id` Vehicle id
* `Component-Id` Component id for parameter
* `Name` Parameter name
* `Value` Parameter value
* `Type` Parameter type using MAVLink `MAV_PARAM_TYPE_*` enum values

A parameter file contains the parameters for a single Vehicle. It can contain parameters for multiple components within that Vehicle.
