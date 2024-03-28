# Mission Command Tree

QGC creates the user interface for editing a specific mission item command dynamically from a hierarchy of json metadata. This hierarchy is called the Mission Command Tree. This way only json metadata must be created when new commands are added.

## Why a tree?

The tree is needed to deal with the idosyncracies of differening firmware supporting commands differently and/or different vehicle types supporting the commands in different ways. The simplest example of that is mavlink spec may include command parameters which are not supported by all firmwares. Or command parameters which are only valid for certain vehicle types. Also in some cases a GCS may decide to hide some of the command parameters from view to end users since they are too complex or cause usability problems.

The tree is the MissionCommandTree class: [MissionCommandTree.cc](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandTree.cc), [MissionCommandTree.h](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandTree.h)

### Tree Root

The root of the tree is json metadata which matches the mavlink spec exactly.

Here you can see an example of the root [json](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoCommon.json#L27) for `MAV_CMD_NAV_WAYPOINT`:

```
        {
            "id":                   16,
            "rawName":              "MAV_CMD_NAV_WAYPOINT",
            "friendlyName":         "Waypoint",
            "description":          "Travel to a position in 3D space.",
            "specifiesCoordinate":  true,
            "friendlyEdit":         true,
            "category":             "Basic",
            "param1": {
                "label":            "Hold",
                "units":            "secs",
                "default":          0,
                "decimalPlaces":    0
            },
            "param2": {
                "label":            "Acceptance",
                "units":            "m",
                "default":          3,
                "decimalPlaces":    2
            },
            "param3": {
                "label":            "PassThru",
                "units":            "m",
                "default":          0,
                "decimalPlaces":    2
            },
            "param4": {
                "label":            "Yaw",
                "units":            "deg",
                "nanUnchanged":     true,
                "default":          null,
                "decimalPlaces":    2
            }
        },
```

Note: In reality this based information should be provided by mavlink itself and not needed to be part of a GCS.

### Leaf Nodes

The leaf nodes then provides metadata which can override values for the command and/or remove parameters from display to the user. The full tree hierarchy is this:

- Root - Generic Mavlink
  - Vehicle Type Specific - Vehicle specific overrides to the generic spec
  - Firmware Type Specific - One optional leaf node for each firmware type (PX4/ArduPilot)
    - Vehicle Type Specific - One optional leaf node for each vehicel type (FW/MR/VTOL/Rover/Sub)

Note: In reality this override capability should be part of mavlink spec and should be able to be queried from the vehicle.

### Building the instance tree from the full tree

Since the json metadata provides information for all firmware/vehicle type combinations the actual tree to use must be built based on the firmware and vehicle type which is being used to create a Plan. This is done through a process call "collapsing" the full tree to a firmware/vehicle specific tree ([code](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandTree.cc#L119)).

The steps are as follows:

- Add the root to the instance tree
- Apply the vehicle type specific overrides to the instance tree
- Apply the firmware type specific overrides to the instance tree
- Apply the firmware/vehicle type specific overrides to the instance tree

The resulting Mission Command Tree is then used to build UI for the Plan View item editors. In reality it is used for more than just that, there are many other places where knowing more information about a specific command id is useful.

## Example hierarchy for `MAV_CMD_NAV_WAYPOINT`

Let's walk through an example hierarchy for `MAV_CMD_NAV_WAYPOINT`. Root information is shown above.

### Root - Vehicle Type Specific leaf node

The next level of the hiearchy is generic mavlink but vehicle specific. Json files are here: [MR](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoMultiRotor.json), [FW](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoFixedWing.json), [ROVER](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoRover.json), [Sub](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoSub.json), [VTOL](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoVTOL.json). And here are the overrides for (Fixed Wings)(https\://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoFixedWing.json#L7):

```
        {
            "id":           16,
            "comment":      "MAV_CMD_NAV_WAYPOINT",
            "paramRemove":  "4"
        },
```

What this does is remove the editing UI for param4 which is Yaw and not used by fixed wings. Since this is a leaf node of the root, this applies to all fixed wing vehicle no matter the firmware type.

### Root - Firmware Type Specific leaf node

The next level of the hiearchy are overrides which are specific to a firmware type but apply to all vehicle types. Once again lets loook at the waypoint overrides:

[ArduPilot](https://github.com/mavlink/qgroundcontrol/blob/master/src/FirmwarePlugin/APM/MavCmdInfoCommon.json#L6):

```
        {
            "id":           16,
            "comment":      "MAV_CMD_NAV_WAYPOINT",
            "paramRemove":  "2"
        },
```

[PX4](https://github.com/mavlink/qgroundcontrol/blob/master/src/FirmwarePlugin/PX4/MavCmdInfoCommon.json#L7):

```
        {
            "id":           16,
            "comment":      "MAV_CMD_NAV_WAYPOINT",
            "paramRemove":  "2,3"
        },
```

You can see that for both firmwares param2 which is acceptance radius is removed from the editing ui. This is a QGC specific decision. It is generally safer and easier to use the firmwares generic acceptance radius handling than the specify a value. So we've decided to hide it from users.

You can also see that for PX4 param3/PassThru is removed since it is not supported by PX.

### Root - Firmware Type Specific - Vehicle Type Specific leaf node

The last level of the hiearchy is both firmware and vehicle type specific.

[ArduPilot/MR](https://github.com/mavlink/qgroundcontrol/blob/master/src/FirmwarePlugin/APM/MavCmdInfoMultiRotor.json#L7):

```
        {
            "id":           16,
            "comment":      "MAV_CMD_NAV_WAYPOINT",
            "paramRemove":  "2,3,4"
        },
```

Here you can see that for an ArduPilot Multi-Rotor vehicle param2/3/4 Acceptance/PassThru/Yaw are removed. Yaw for example is removed because it is not supported. Due to quirk of how this code works, you need to repeat the overrides from the lower level.

## Mission Command UI Info

Two classes define the metadata associated with a command:

- MissionCommandUIInfo - Metadata for the entire command
- MissionCmdParamInfo - Metadata for a param in a command

The source is commented with full details of the json keys which are supported.

[MissionCommandUIInfo](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandUIInfo.h#L82):

```
/// UI Information associated with a mission command (MAV_CMD)
///
/// MissionCommandUIInfo is used to automatically generate editing ui for a MAV_CMD. This object also supports the concept of only having a set of partial
/// information for the command. This is used to create overrides of the base command information. For on override just specify the keys you want to modify
/// from the base command ui info. To override param ui info you must specify the entire MissionParamInfo object.
///
/// The json format for a MissionCommandUIInfo object is:
///
/// Key                     Type    Default     Description
/// id                      int     reauired    MAV_CMD id
/// comment                 string              Used to add a comment
/// rawName                 string  required    MAV_CMD enum name, should only be set of base tree information
/// friendlyName            string  rawName     Short description of command
/// description             string              Long description of command
/// specifiesCoordinate     bool    false       true: Command specifies a lat/lon/alt coordinate
/// specifiesAltitudeOnly   bool    false       true: Command specifies an altitude only (no coordinate)
/// standaloneCoordinate    bool    false       true: Vehicle does not fly through coordinate associated with command (exampl: ROI)
/// isLandCommand           bool    false       true: Command specifies a land command (LAND, VTOL_LAND, ...)
/// friendlyEdit            bool    false       true: Command supports friendly editing dialog, false: Command supports 'Show all values" style editing only
/// category                string  Advanced    Category which this command belongs to
/// paramRemove             string              Used by an override to remove params, example: "1,3" will remove params 1 and 3 on the override
/// param[1-7]              object              MissionCommandParamInfo object
///
```

[MissionCmdParamInfo](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandUIInfo.h#L25):

```
/// UI Information associated with a mission command (MAV_CMD) parameter
///
/// MissionCommandParamInfo is used to automatically generate editing ui for a parameter associated with a MAV_CMD.
///
/// The json format for a MissionCmdParamInfo object is:
///
/// Key             Type    Default     Description
/// label           string  required    Label for text field
/// units           string              Units for value, should use FactMetaData units strings in order to get automatic translation
/// default         double  0.0/NaN     Default value for param. If no default value specified and nanUnchanged == true, then defaultValue is NaN.
/// decimalPlaces   int     7           Number of decimal places to show for value
/// enumStrings     string              Strings to show in combo box for selection
/// enumValues      string              Values associated with each enum string
/// nanUnchanged    bool    false       True: value can be set to NaN to signal unchanged
```
