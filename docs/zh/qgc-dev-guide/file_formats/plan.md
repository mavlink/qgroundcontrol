# 计划文件格式

计划文件以JSON文件格式存储，包含任务项和（可选）地理围栏和集结点。 您可以在下面看到Plan文件的顶级格式
地理围栏“多边形”计划格式的版本号。 记录的版本是2。

:::tip
**提示：** 这是“接近最小” - 计划必须包含至少一个任务项目。 当没有任务运行时，计划围栏和集结点也用于模式。
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

主要领域是：

| 键                              | 描述                                                |
| ------------------------------ | ------------------------------------------------- |
| `version`                      | 地理围栏计划格式的版本号。 记录的版本是2。 地理围栏“圈子”计划格式的版本号。 记录的版本是1。 |
| `fileType`                     | 必须是“计划”。                                          |
| `groundStation`                | 创建此文件的地面站的名称（此处为QGroundControl）                   |
| [`mission`](#mission)          | 与此飞行计划相关的任务。                                      |
| [`geoFence`](#geofence)        | （可选）此计划的地理围栏信息。                                   |
| [`rallyPoints`](#rally_points) | （可选）此计划的拉力/安全点信息                                  |

## 使命对象 {#mission}

The structure of the mission object is shown below.
任务对象的结构如下所示。 items字段包含以逗号分隔的任务项列表（它必须包含至少一个任务项，如下所示）。 该列表可以是SimpleItem和ComplexItem对象的混合
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

需要以下值：

| 键                        | 描述                                                                                                                                                                                                                                                               |
| ------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `version`                | 任务对象的版本。 目前的版本是2。 Current version is 2.                                                                                                                                                                                                          |
| `firmwareType`           | 为此任务创建的固件类型。 这是MAV_AUTOPILOT枚举值之一。 This is one of the [MAV_AUTOPILOT](https://mavlink.io/en/messages/common.html#MAV_AUTOPILOT) enum values.                                                           |
| `globalPlanAltitudeMode` | The global plan-wide altitude mode setting. This is used by plan items that don't specify an `"AltitudeMode"`.                                                                                                                   |
| `vehicleType`            | The vehicle type for which this mission was created. This is one of the [MAV_TYPE](https://mavlink.io/en/messages/common.html#MAV_TYPE) enum values.                                                        |
| `cruiseSpeed`            | The default forward speed for Fixed wing or VTOL vehicles (i.e. when moving between waypoints).                                                                                               |
| `hoverSpeed`             | The default forward speed for multi-rotor vehicles.                                                                                                                                                                                              |
| `items`                  | The list of mission item objects associated with the mission . The list may contain either/both [SimpleItem](#mission_simple_item) and [ComplexItem](#mission_complex_item) objects.                                             |
| `plannedHomePosition`    | The planned home position is shown on the map and used for mission planning when no vehicle is connected. The array values shown above are (from top): latitude, longitude and AMSL altitude. |

简单和复杂项目的格式如下。

### SimpleItem - 简单的任务项目 {#mission_simple_item}

一个简单的项表示单个MAVLink MISSION_ITEM命令。

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

字段映射如下所示。

| 键                     | 描述                                                                                                                                                                                                                                                                                    |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `type`                | SimpleItem用于简单的项目                                                                                                                                                                                                                                                                     |
| `AMSLAltAboveTerrain` | 向用户显示的海拔高度值。                                                                                                                                                                                                                                                                          |
| `Altitude`            |                                                                                                                                                                                                                                                                                       |
| `AltitudeMode`        |                                                                                                                                                                                                                                                                                       |
| `autoContinue`        | MISSION_ITEM.autoContinue                                                                                                                                                                                                                        |
| `command`             | The command ([MAV_CMD](https://mavlink.io/en/messages/common.html#MAV_CMD)) for this mission item - see [MISSION_ITEM](https://mavlink.io/en/messages/common.html#MISSION_ITEM).command. |
| `doJumpId`            | DO_JUMP命令中当前任务项的目标ID。 这些是从1自动编号。 These are auto-numbered from 1.                                                                                                                                                                                 |
| `frame`               | [MAV_FRAME](https://mavlink.io/en/messages/common.html#MAV_FRAME) (see [MISSION_ITEM](https://mavlink.io/en/messages/common.html#MISSION_ITEM).frame)                                                    |
| `params`              | MISSION_ITEM.param1,2,3,4，x，y，z（值取决于特定的MAV_CMD）。                                                                                                                                                                            |

### 复杂任务项目 {#mission_complex_item}

复杂项是对作为单个实体处理的多个MISSION_ITEM对象的更高级别封装。

目前有三种类型的复杂任务项目：

- 调查](#survey)
- [走廊扫描](#corridor_scan)
- [结构扫描](#structure_scan)

#### 调查 - 复杂任务项目 {#survey}

调查复杂任务项目的对象定义如下。

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

复杂项目具有与之关联的这些值：

| 键                                                        | 描述                                                                                                                                                                                                                       |
| -------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `version`                                                | The version number for this `survey` definition. Current version is 3.                                                                                                                   |
| `type`                                                   | ComplexItem（这是一个复杂的项目）。                                                                                                                                                                                                  |
| `complexItemType`                                        | `survey`                                                                                                                                                                                                                 |
| [`TransectStyleComplexItem	`](#TransectStyleComplexItem) | Survey和CorridorScan复杂项目的通用基础定义。                                                                                                                                                                                          |
| `angle`                                                  | 横断面的角度（度数）。                                                                                                                                                                                                              |
| `entryLocation`                                          | ?                                                                                                                                                                                                                        |
| `flyAlternateTransects`                                  | 团结积分信息是可选的。 该计划可以包含任意数量的拉力点，每个拉力点具有纬度，经度和高度（高于原始位置）。 如果是，则载具会跳过每个其他横断面，然后在最后返回并飞行这些替代。 This can be used for fixed wing aircraft when the turnaround would be too acute for the vehicle to make the turn. |
| `polygon`                                                | The polygon array which represents the polygonal survey area. Each point is a latitude, longitude pair for a polygon vertex.                                                             |

#### 走廊扫描 {#corridor_scan}

CorridorScan复杂任务项的对象定义如下。

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

| 键                                                        | 描述                                                                 |
| -------------------------------------------------------- | ------------------------------------------------------------------ |
| `version`                                                | 此CorridorScan定义的版本。 目前的版本是3。 Current version is 3. |
| `type`                                                   | ComplexItem（这是一个复杂的项目）。                                            |
| `complexItemType`                                        | `CorridorScan`                                                     |
| `CorridorWidth`                                          | ?                                                                  |
| `EntryPoint`                                             | ?                                                                  |
| [`TransectStyleComplexItem	`](#TransectStyleComplexItem) | Survey和CorridorScan复杂项目的通用基础定义。                                    |
| `polyline`                                               | ?                                                                  |

#### 结构扫描 {#structure_scan}

StructureScan复杂任务项的对象定义如下。

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

| 键                           | 描述                                                                        |
| --------------------------- | ------------------------------------------------------------------------- |
| `version`                   | 此StructureScan定义的版本。 目前的版本是2。 Current version is 2.       |
| `type`                      | ComplexItem（这是一个复杂的项目）。                                                   |
| `complexItemType`           | `StructureScan`                                                           |
| `Altitude`                  | ?                                                                         |
| [`CameraCalc`](#CameraCalc) | ?                                                                         |
| `Layers`                    | ?                                                                         |
| `StructureHeight`           | ?                                                                         |
| `altitudeRelative`          | true: altitude相对于主页，false: altitude是AMSL。 |
| `polygon`                   | ?                                                                         |

#### `TransectStyleComplexItem	` {#TransectStyleComplexItem}

TransectStyleComplexItem contains the common base definition for survey and CorridorScan complex items.

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

| 键                            | 描述                                                      |
| ---------------------------- | ------------------------------------------------------- |
| `version`                    | 此TransectStyleComplexItem定义的版本。 当前版本为1。 此文件的版本。 当前版本为1。 |
| [`CameraCalc`](#CameraCalc)  | ?                                                       |
| `CameraTriggerInTurnAround	` | ? （布尔值）                                                 |
| `FollowTerrain`              | ? （布尔值）                                                 |
| `HoverAndCapture`            | ? （布尔值）                                                 |
| `Items`                      | ?                                                       |
| `Refly90Degrees`             | ? （布尔值）                                                 |
| `TurnAroundDistance`         | 在转向下一个横断面之前飞过多边形边缘的距离。                                  |
| `VisualTransectPoints`       | ?                                                       |

##### CameraCalc {#CameraCalc}

CameraCalc包含用于调查，走廊或结构扫描的摄像机信息。

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

| 键                           | 描述                                                                                                                                                                                                                                                                                                       |
| --------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `version`                   | 此CameraCalc定义的版本。 当前版本为1。 此调查定义的版本号。 目前的版本是3。                                                                                                                                                                                                                                                            |
| `AdjustedFootprintFrontal`  | ?                                                                                                                                                                                                                                                                                                        |
| `AdjustedFootprintSide`     | ?                                                                                                                                                                                                                                                                                                        |
| `DistanceToSurface`         | ? 单位?                                                                                                                                                                                                                                                                                                    |
| `DistanceToSurfaceRelative` | ?                                                                                                                                                                                                                                                                                                        |
| `CameraName`                | 正在使用的摄像机名称（必须对应于QGroundControl已知的摄像机之一或： Manual (no camera specs) (手动（无摄像机规格）用于手动设置， Custom Camera (自定义摄像机)用于自定义设置。 未在“手动”摄像机定义中指定此点后列出的键。 The keys listed after this point are not specified for a "Manual" camera definition. |
| `FixedOrientation`          | ? （布尔值）                                                                                                                                                                                                                                                                                                  |
| `FocalLength`               | 相机镜头的焦距，以毫米为单位。                                                                                                                                                                                                                                                                                          |
| `FrontalOverlap`            | 正面图像重叠的百分比。                                                                                                                                                                                                                                                                                              |
| `ImageDensity`              | ?                                                                                                                                                                                                                                                                                                        |
| `ImageHeight`               | 图像高度以px为单位                                                                                                                                                                                                                                                                                               |
| `ImageWidth`                | 图像宽度以px为单位                                                                                                                                                                                                                                                                                               |
| `景观`                        | true：相机以横向方向安装在载具上，false：相机以纵向方向安装在载具上。                                                                                                                                                                                                                                                                  |
| `MinTriggerInterval`        | ?                                                                                                                                                                                                                                                                                                        |
| `SensorHeight`              | 传感器高度，以毫米为单位。                                                                                                                                                                                                                                                                                            |
| `SensorWidth`               | 传感器宽度，以毫米为单位。                                                                                                                                                                                                                                                                                            |
| `SideOverlap`               | 侧面图像重叠的百分比。                                                                                                                                                                                                                                                                                              |
| `ValueSetIsDistance`        | ? （布尔值）                                                                                                                                                                                                                                                                                                  |

## GeoFence (地理围栏) {#geofence}

Geofence information is optional.
The plan can contain an arbitrary number of geofences defined in terms of polygons and circles.

最小定义如下所示。

```json
     "geoFence": {
        "circles": [
        ],
        "polygons": [
        ],
        "version": 2
    },
```

主要领域是：

| 键                              | 描述                                                                                                            |
| ------------------------------ | ------------------------------------------------------------------------------------------------------------- |
| `version`                      | The version number for the geofence plan format. The documented version is 2. |
| [`circles`](#circle_geofence)  | 包含圆形地理围栏定义的列表（以逗号分隔）。                                                                                         |
| [`polygon`](#polygon_geofence) | 包含多边形地理围栏定义的列表（以逗号分隔）。                                                                                        |

### Circle Geofence (圆形地理围栏) {#circle_geofence}

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

主要领域是：

| 键           | 描述                                                                                                                                                              |
| ----------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `version`   | The version number for the geofence "circle" plan format. 拉力点计划格式的版本号。 记录的版本是2。                                                                 |
| `circle`    | 圆的定义。 包括 centre (中心)（纬度，经度）和半径，如上所示。 Includes `centre` (latitude, longitude) and `radisu` as shown above. |
| `inclusion` | 地理围栏是否已启用（true）或已禁用。                                                                                                                                            |

### 多边形地理围栏 {#polygon_geofence}

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

这些领域是：

| 键           | 描述                                                                                                                                                                             |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `version`   | The version number for the geofence "polygon" plan format. The documented version is 2.                                                        |
| `polygon`   | A list of points for the polygon. Each point contains a latitude and longitude. The points are ordered in a clockwise winding. |
| `inclusion` | 地理围栏是否已启用（true）或已禁用。                                                                                                                                                           |

## 团结积分 {#rally_points}

Rally point information is optional.
The plan can contain an arbitrary number of rally points, each of which has a latitude, longitude, and altitude (above home position).

有两点的定义如下所示。

```json
<br />    "rallyPoints": {
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

这些领域是：

| 键         | 描述                                                                                                               |
| --------- | ---------------------------------------------------------------------------------------------------------------- |
| `version` | The version number for the rally point plan format. The documented version is 2. |
| `points`  | 拉力点列表。                                                                                                           |
