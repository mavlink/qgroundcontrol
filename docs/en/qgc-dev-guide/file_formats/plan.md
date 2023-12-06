# Plan File Format

Plan files are stored in JSON file format and contain mission items and (optional) geo-fence and rally-points.
Below you can see the top level format of a Plan file

::: tip
This is "near-minimal" - a plan must contain at least one mission item.
The plan fence and rally points are also used in modes when no mission is running.
:::

```json
{
  "fileType": "Plan",
  "geoFence": {
    "circles": [],
    "polygons": [],
    "version": 2
  },
  "groundStation": "QGroundControl",
  "mission": {},
  "rallyPoints": {
    "points": [],
    "version": 2
  },
  "version": 1
}
```

The main fields are:

| Key                            | Description                                                                    |
| ------------------------------ | ------------------------------------------------------------------------------ |
| `version`                      | The version for this file. Current version is 1.                               |
| `fileType`                     | Must be `"Plan"`.                                                              |
| `groundStation`                | The name of the ground station which created this file (here _QGroundControl_) |
| [`mission`](#mission)          | The mission associated with this flight plan.                                  |
| [`geoFence`](#geofence)        | (Optional) Geofence information for this plan.                                 |
| [`rallyPoints`](#rally_points) | (Optional) Rally/Safe point information for this plan                          |

## Mission Object {#mission}

The structure of the mission object is shown below.
The `items` field contains a comma-separated list of mission items (it must contain at least one mission item, as shown below).
The list may be a mix of both [SimpleItem](#mission_simple_item) and [ComplexItem](#mission_complex_item) objects.

```json
    "mission": {
        "cruiseSpeed": 15,
        "firmwareType": 12,
        "globalPlanAltitudeMode": 1,
        "hoverSpeed": 5,
        "items": [
            {
                "AMSLAltAboveTerrain": null,
                "Altitude": 50,
                "AltitudeMode": 0,
                "autoContinue": true,
                "command": 22,
                "doJumpId": 1,
                "frame": 3,
                "params": [
                    15,
                    0,
                    0,
                    null,
                    47.3985099,
                    8.5451002,
                    50
                ],
                "type": "SimpleItem"
            }
        ],
        "plannedHomePosition": [
            47.3977419,
            8.545594,
            487.989
        ],
        "vehicleType": 2,
        "version": 2
    },
```

The following values are required:

| Key                      | Description                                                                                                                                                                                   |
| ------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `version`                | The version for the mission object. Current version is 2.                                                                                                                                     |
| `firmwareType`           | The firmware type for which this mission was created. This is one of the [MAV_AUTOPILOT](https://mavlink.io/en/messages/common.html#MAV_AUTOPILOT) enum values.                               |
| `globalPlanAltitudeMode` | The global plan-wide altitude mode setting. This is used by plan items that don't specify an `"AltitudeMode"`.                                                                                |
| `vehicleType`            | The vehicle type for which this mission was created. This is one of the [MAV_TYPE](https://mavlink.io/en/messages/common.html#MAV_TYPE) enum values.                                          |
| `cruiseSpeed`            | The default forward speed for Fixed wing or VTOL vehicles (i.e. when moving between waypoints).                                                                                               |
| `hoverSpeed`             | The default forward speed for multi-rotor vehicles.                                                                                                                                           |
| `items`                  | The list of mission item objects associated with the mission . The list may contain either/both [SimpleItem](#mission_simple_item) and [ComplexItem](#mission_complex_item) objects.          |
| `plannedHomePosition`    | The planned home position is shown on the map and used for mission planning when no vehicle is connected. The array values shown above are (from top): latitude, longitude and AMSL altitude. |

The format of the simple and complex items is given below.

### `SimpleItem` - Simple Mission Item {#mission_simple_item}

A simple item represents a single MAVLink [MISSION_ITEM](https://mavlink.io/en/messages/common.html#MISSION_ITEM) command.

```
            {
                "AMSLAltAboveTerrain": null,
                "Altitude": 50,
                "AltitudeMode": 0,
                "autoContinue": true,
                "command": 22,
                "doJumpId": 1,
                "frame": 3,
                "params": [
                    15,
                    0,
                    0,
                    null,
                    47.3985099,
                    8.5451002,
                    50
                ],
                "type": "SimpleItem"
            }
```

The field mapping is shown below.

| Key                   | Description                                                                                                                                                                                  |
| --------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `type`                | `SimpleItem` for a simple item                                                                                                                                                               |
| `AMSLAltAboveTerrain` | Altitude value shown to the user.                                                                                                                                                            |
| `Altitude`            |
| `AltitudeMode`        |
| `autoContinue`        | [MISSION_ITEM](https://mavlink.io/en/messages/common.html#MISSION_ITEM).autoContinue                                                                                                         |
| `command`             | The command ([MAV_CMD](https://mavlink.io/en/messages/common.html#MAV_CMD)) for this mission item - see [MISSION_ITEM](https://mavlink.io/en/messages/common.html#MISSION_ITEM).command.     |
| `doJumpId`            | The target id for the current mission item in DO_JUMP commands. These are auto-numbered from 1.                                                                                              |
| `frame`               | [MAV_FRAME](https://mavlink.io/en/messages/common.html#MAV_FRAME) (see [MISSION_ITEM](https://mavlink.io/en/messages/common.html#MISSION_ITEM).frame)                                        |
| `params`              | [MISSION_ITEM](https://mavlink.io/en/messages/common.html#MISSION_ITEM).param1,2,3,4,x,y,z (values depends on the particular [MAV_CMD](https://mavlink.io/en/messages/common.html#MAV_CMD)). |

### Complex Mission Item {#mission_complex_item}

A complex item is a higher level encapsulation of multiple `MISSION_ITEM` objects treated as a single entity.

There are currently three types of complex mission items:

- [Survey](#survey)
- [Corridor Scan](#corridor_scan)
- [Structure Scan](#structure_scan)

#### Survey - Complex Mission Item {#survey}

The object definition for a `Survey` complex mission item is given below.

```
{
                "TransectStyleComplexItem": {
                    ...
                },
                "angle": 0,
                "complexItemType": "survey",
                "entryLocation": 0,
                "flyAlternateTransects": false,
                "polygon": [
                    [
                        -37.75170619863631,
                        144.98414811224316
                    ],
                    ...
                    [
                        -37.75170619863631,
                        144.99457681259048
                    ]
                ],
                "type": "ComplexItem",
                "version": 4
            },
```

Complex items have these values associated with them:

| Key                                                     | Description                                                                                                                                                                                                                   |
| ------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `version`                                               | The version number for this `survey` definition. Current version is 3.                                                                                                                                                        |
| `type`                                                  | `ComplexItem` (this is a complex item).                                                                                                                                                                                       |
| `complexItemType`                                       | `survey`                                                                                                                                                                                                                      |
| [`TransectStyleComplexItem`](#TransectStyleComplexItem) | The common base definition for Survey and CorridorScan complex items.                                                                                                                                                         |
| `angle`                                                 | The angle for the transect paths (degrees).                                                                                                                                                                                   |
| `entryLocation`                                         | ?                                                                                                                                                                                                                             |
| `flyAlternateTransects`                                 | If true, the vehicle will skip every other transect and then come back at the end and fly these alternates. This can be used for fixed wing aircraft when the turnaround would be too acute for the vehicle to make the turn. |
| `polygon`                                               | The polygon array which represents the polygonal survey area. Each point is a latitude, longitude pair for a polygon vertex.                                                                                                  |

#### Corridor Scan {#corridor_scan}

The object definition for a `CorridorScan` complex mission item is given below.

```
            {
                "CorridorWidth": 50,
                "EntryPoint": 0,
                "TransectStyleComplexItem": {
                    ...
                    },
                },
                "complexItemType": "CorridorScan",
                "polyline": [
                    [
                        -37.75234887156983,
                        144.9893624624168
                    ],
                    ...
                    [
                        -37.75491914850321,
                        144.9893624624168
                    ]
                ],
                "type": "ComplexItem",
                "version": 2
            },
```

| Key                                                     | Description                                                           |
| ------------------------------------------------------- | --------------------------------------------------------------------- |
| `version`                                               | The version for this `CorridorScan` definition. Current version is 3. |
| `type`                                                  | `ComplexItem` (this is a complex item).                               |
| `complexItemType`                                       | `CorridorScan`                                                        |
| `CorridorWidth`                                         | ?                                                                     |
| `EntryPoint`                                            | ?                                                                     |
| [`TransectStyleComplexItem`](#TransectStyleComplexItem) | The common base definition for Survey and CorridorScan complex items. |
| `polyline`                                              | ?                                                                     |

#### Structure Scan {#structure_scan}

The object definition for a `StructureScan` complex mission item is given below.

```json
            {
                "Altitude": 50,
                "CameraCalc": {
                    "AdjustedFootprintFrontal": 25,
                    "AdjustedFootprintSide": 25,
                    "CameraName": "Manual (no camera specs)",
                    "DistanceToSurface": 10,
                    "DistanceToSurfaceRelative": true,
                    "version": 1
                },
                "Layers": 1,
                "StructureHeight": 25,
                "altitudeRelative": true,
                "complexItemType": "StructureScan",
                "polygon": [
                    [
                        -37.753184359536355,
                        144.98879374063998
                    ],
                    ...
                    [
                        -37.75408368012594,
                        144.98879374063998
                    ]
                ],
                "type": "ComplexItem",
                "version": 2
            }
```

| Key                         | Description                                                            |
| --------------------------- | ---------------------------------------------------------------------- |
| `version`                   | The version for this `StructureScan` definition. Current version is 2. |
| `type`                      | `ComplexItem` (this is a complex item).                                |
| `complexItemType`           | `StructureScan`                                                        |
| `Altitude`                  | ?                                                                      |
| [`CameraCalc`](#CameraCalc) | ?                                                                      |
| `Layers`                    | ?                                                                      |
| `StructureHeight`           | ?                                                                      |
| `altitudeRelative`          | `true`: `altitude` is relative to home, `false`: `altitude` is AMSL.   |
| `polygon`                   | ?                                                                      |

#### `TransectStyleComplexItem` {#TransectStyleComplexItem}

`TransectStyleComplexItem` contains the common base definition for [`survey`](#survey) and [`CorridorScan`](#CorridorScan) complex items.

```json
                "TransectStyleComplexItem": {
                    "CameraCalc": {
                        ...
                    },
                    "CameraTriggerInTurnAround": true,
                    "FollowTerrain": false,
                    "HoverAndCapture": false,
                    "Items": [
                        ...
                    ],
                    "Refly90Degrees": false,
                    "TurnAroundDistance": 10,
                    "VisualTransectPoints": [
                        [
                            -37.75161626657736,
                            144.98414811224316
                        ],
                        ...
                        [
                            -37.75565155437309,
                            144.99438539496475
                        ]
                    ],
                    "version": 1
                },
```

| Key                         | Description                                                                       |
| --------------------------- | --------------------------------------------------------------------------------- |
| `version`                   | The version for this `TransectStyleComplexItem` definition. Current version is 1. |
| [`CameraCalc`](#CameraCalc) | ?                                                                                 |
| `CameraTriggerInTurnAround` | ? (boolean)                                                                       |
| `FollowTerrain`             | ? (boolean)                                                                       |
| `HoverAndCapture`           | ? (boolean)                                                                       |
| `Items`                     | ?                                                                                 |
| `Refly90Degrees`            | ? (boolean)                                                                       |
| `TurnAroundDistance`        | The distance to fly past the polygon edge prior to turning for the next transect. |
| `VisualTransectPoints`      | ?                                                                                 |

##### CameraCalc {#CameraCalc}

The `CameraCalc` contains camera information used for a survey, corridor or structure scan.

```
                    "CameraCalc": {
                        "AdjustedFootprintFrontal": 272.4,
                        "AdjustedFootprintSide": 409.2,
                        "CameraName": "Sony ILCE-QX1",
                        "DistanceToSurface": 940.6896551724138,
                        "DistanceToSurfaceRelative": true,
                        "FixedOrientation": false,
                        "FocalLength": 16,
                        "FrontalOverlap": 70,
                        "ImageDensity": 25,
                        "ImageHeight": 3632,
                        "ImageWidth": 5456,
                        "Landscape": true,
                        "MinTriggerInterval": 0,
                        "SensorHeight": 15.4,
                        "SensorWidth": 23.2,
                        "SideOverlap": 70,
                        "ValueSetIsDistance": false,
                        "version": 1
                    },
```

| Key                         | Description                                                                                                                                                                                                                                                          |
| --------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `version`                   | The version for this `CameraCalc` definition. Current version is 1.                                                                                                                                                                                                  |
| `AdjustedFootprintFrontal`  | ?                                                                                                                                                                                                                                                                    |
| `AdjustedFootprintSide`     | ?                                                                                                                                                                                                                                                                    |
| `DistanceToSurface`         | ? Units?                                                                                                                                                                                                                                                             |
| `DistanceToSurfaceRelative` | ?                                                                                                                                                                                                                                                                    |
| `CameraName`                | Name of camera being used (must correspond to one of the cameras known to _QGroundControl_ or: `Manual (no camera specs)` for manual setup, `Custom Camera` for a custom setup. The keys listed after this point are not specified for a "Manual" camera definition. |
| `FixedOrientation`          | ? (boolean)                                                                                                                                                                                                                                                          |
| `FocalLength`               | Focal length of camera lens in millimeters.                                                                                                                                                                                                                          |
| `FrontalOverlap`            | Percentage of frontal image overlap.                                                                                                                                                                                                                                 |
| `ImageDensity`              | ?                                                                                                                                                                                                                                                                    |
| `ImageHeight`               | Image height in px                                                                                                                                                                                                                                                   |
| `ImageWidth`                | Image width in px                                                                                                                                                                                                                                                    |
| `Landscape`                 | `true`: Camera installed in landscape orientation on vehicle, `false`: Camera installed in portrait orientation on vehicle.                                                                                                                                          |
| `MinTriggerInterval`        | ?                                                                                                                                                                                                                                                                    |
| `SensorHeight`              | Sensor height in millimeters.                                                                                                                                                                                                                                        |
| `SensorWidth`               | Sensor width in millimeters.                                                                                                                                                                                                                                         |
| `SideOverlap`               | Percentage of side image overlap.                                                                                                                                                                                                                                    |
| `ValueSetIsDistance`        | ? (boolean)                                                                                                                                                                                                                                                          |

## GeoFence {#geofence}

Geofence information is optional.
The plan can contain an arbitrary number of geofences defined in terms of polygons and circles.

The minimal definition is shown below.

```json
    "geoFence": {
        "circles": [
        ],
        "polygons": [
        ],
        "version": 2
    },
```

The fields are:

| Key                             | Description                                                                   |
| ------------------------------- | ----------------------------------------------------------------------------- |
| `version`                       | The version number for the geofence plan format. The documented version is 2. |
| [`circles`](#circle_geofence)   | List containing circle geofence definitions (comma separated).                |
| [`polygons`](#polygon_geofence) | List containing polygon geofence definitions (comma separated).               |

### Circle Geofence {#circle_geofence}

Each circular geofence is defined in a separate item, as shown below (multiple comma-separated items can be defined).
The items define the centre and radius of the circle, and whether or not the specific geofence is activated.

```json
{
  "circle": {
    "center": [47.39756763610029, 8.544649762407738],
    "radius": 319.85
  },
  "inclusion": true,
  "version": 1
}
```

The fields are:

| Key         | Description                                                                                        |
| ----------- | -------------------------------------------------------------------------------------------------- |
| `version`   | The version number for the geofence "circle" plan format. The documented version is 1.             |
| `circle`    | The definition of the circle. Includes `centre` (latitude, longitude) and `radisu` as shown above. |
| `inclusion` | Whether or not the geofence is enabled (true) or disabled.                                         |

### Polygon Geofence {#polygon_geofence}

Each polygon geofence is defined in a separate item, as shown below (multiple comma-separated items can be defined).
The geofence includes a set of points defined with a clockwise winding (i.e. they must enclose an area).

```json
            {
                "inclusion": true,
                "polygon": [
                    [
                        47.39807773798406,
                        8.543834631785785
                    ],
                    [
                        47.39983519888905,
                        8.550024648373267
                    ],
                    [
                        47.39641100087146,
                        8.54499282423751
                    ],
                    [
                        47.395590322265186,
                        8.539435808992085
                    ]
                ],
                "version": 1
            }
        ],
        "version": 2
    }
```

The fields are:

| Key         | Description                                                                                                                    |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------ |
| `version`   | The version number for the geofence "polygon" plan format. The documented version is 2.                                        |
| `polygon`   | A list of points for the polygon. Each point contains a latitude and longitude. The points are ordered in a clockwise winding. |
| `inclusion` | Whether or not the geofence is enabled (true) or disabled.                                                                     |

## Rally Points {#rally_points}

Rally point information is optional.
The plan can contain an arbitrary number of rally points, each of which has a latitude, longitude, and altitude (above home position).

A definition with two points is shown below.

```json

    "rallyPoints": {
        "points": [
            [
                47.39760401,
                8.5509154,
                50
            ],
            [
                47.39902017,
                8.54263274,
                50
            ]
        ],
        "version": 2
    }
```

The fields are:

| Key       | Description                                                                      |
| --------- | -------------------------------------------------------------------------------- |
| `version` | The version number for the rally point plan format. The documented version is 2. |
| `points`  | A list of rally points.                                                          |
