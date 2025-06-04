# 任务命令树

QGC 创建用户界面，动态地从json 元数据层次编辑特定的任务项命令。 此层次结构称为任务命令树。 这种方式只能在添加新命令时创建 json 元数据。

## 为什么是树型结构？

不同的固件支持不同的命令，以及/或者不同类型的载具以不同的方式支持不同的命令，因此需要使用树形结构来处理这些问题。 最简单的例子是 mavlink 规范可能包含了并非所有固件都支持的命令参数。 或仅适用于某些类型载具的命令参数。 此外，在某些情况下，地面控制站可能会决定将某些命令参数在视图中对最终用户进行隐藏，因为它们过于复杂或导致可用性问题。

该树是 MissionCommandTree 类：[MissionCommandTree.cc](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandTree.cc), [MissionCommandTree.h](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandTree.h)

### 树根目录

树的根目录是与 mavlink 规范完全匹配的json元数据。

下面是 MAV_CMD_NAV_WAYPOINT 的根 [json](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoCommon.json#L27) 示例：

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

The leaf nodes then provides metadata which can override values for the command and/or remove parameters from display to the user. 完整的树层次结构是这样：

- 根-通用Mavlink
  - 特定的载具类型－特定载具的通用规范
  - 固件类型特定——每种固件类型的一个可选叶节点 (PX4/ArduPilot)
    - 特定载具型号——每种载具类型有一个可选的叶节点（固定翼/多旋翼/垂直起降飞行器（VTOL）/小车/潜艇）

注意：实际上，此替代功能应该是mavlink规格的一部分，并且应该可以从载具中查询。

### 从完整树中构建实例树

由于json元数据提供了所有固件/载具类型组合的信息，实际使用的树必须建立在用于创建计划的固件和载具类型的基础上。 这是通过一个进程叫做“折叠”完整树到一个固件/载具特定树([code](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandTree.cc#L119))来完成的。

步骤如下：

- 将根添加到实例树
- 将特定的载具类型重写实例树
- 对实例树应用固件类型特定覆盖
- 对实例树应用固件/载具类型特定覆盖

然后，生成的任务命令树将为平面项目编辑器构建UI。 实际上，它不仅用于此，还有许多其他地方可以帮助您了解有关特定命令 id 的更多信息。

## `MAV_CMD_NAV_WAYPOINT` 示例层次结构

让我们来看看 `MAV_CMD_NAV_WAYPOINT` 的示例层次结构。 根信息如上图所示。

### 根-载具类型特定叶节点

层次结构的下一个层级是通用的 mavlink，但只针对特定的载具。 Json 文件在这里: [多旋翼](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoMultiRotor.json)，[固定翼](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoRover.json)，[小车](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoRover.json)，[潜艇](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoSub.json)， [垂直起降飞行器（VTOL）](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoVTOL.json)。 这里是重写 (固定Wings) (https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoFixedWing.json#L7):

```
        {
            "id":           16,
            "comment":      "MAV_CMD_NAV_WAYPOINT",
            "paramRemove":  "4"
        },
```

这样做是删除参数4的编辑界面，固定翼没有使用航向（Yaw）参数。 由于这是根的叶节点，因此无论固件类型如何，这都适用于所有固定翼载具。

### 根－载具类型的特定叶节点

下一层厚此薄彼会覆盖固件类型特定但适用于所有载具类型。 让我们再次看看航点重写功能：

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

您可以看到，对于两个固件参数参数2，即接受半径，从编辑 ui 中删除。 这是QGC的特性决定。 与指定值相比，使用固件通用接受半径会更加安全和容易。 因此，我们决定对用户隐藏它。

您还可以看到，对于 PX4 param3/PassThru，由于 PX 不支持它，因此已被删除。

### 根－特定于固件的类型－特定于载具类型的叶子节点

层次结构的最后一个级别既针对固件又针对载具类型。

[ArduPilot/MR](https://github.com/mavlink/qgroundcontrol/blob/master/src/FirmwarePlugin/APM/MavCmdInfoMultiRotor.json#L7)：

```
        {
            "id":           16,
            "comment":      "MAV_CMD_NAV_WAYPOINT",
            "paramRemove":  "2,3,4"
        },
```

在这里你可以看到，ArduPilot的多电机载具参数2/3/4 Acceptance/PassThru/Yaw 已被移除。 例如，航向（Yaw）是因为不支持所以被移除。 由于这个代码如何工作的问题，您需要从较低级别重复重写。

## 任务命令界面信息

两个类定义与命令相关联的元数据：

- MissionCommandUIInfo－整个命令的元数据
- MissionCmdParamInfo－命令中参数的元数据

源中注释了支持 json 键的完整详细信息。

[任务指令信息](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandUIInfo.h#L82):

```
/// 与任务指令（MAV_CMD）相关联的用户界面信息
///
/// MissionCommandUIInfo 用于为 MAV_CMD 自动生成编辑用户界面。
/// 此对象还支持仅针对指令拥有一组部分信息的概念。这用于创建对基本指令信息的覆盖。
/// 对于覆盖，只需指定你想要从基本指令用户界面信息中修改的键。
/// 要覆盖参数用户界面信息，你必须指定整个 MissionParamInfo 对象。
///
///  MissionCommandUIInfo 对象的 JSON 格式如下：
///
/// 键                      类型     默认值      描述
/// id                      int     必填项       MAV_CMD id
/// comment                 string              用于添加注释
/// rawName                 string  必填项       MAV_CMD 枚举名称，应仅在基本树信息中设置
/// friendlyName            string  rawName     指令的简短描述
/// description             string              指令的详细描述
/// specifiesCoordinate     bool    false       true: 指令指定经纬度 / 高度坐标
/// specifiesAltitudeOnly   bool    false       true: 指令仅指定高度（无坐标）
/// standaloneCoordinate    bool    false       true: 飞行器不会飞越与指令相关的坐标（例如：兴趣点（ROI））
/// isLandCommand           bool    false       true: 指令指定着陆指令（LAND、VTOL_LAND 等）
/// friendlyEdit            bool    false       true: 指令支持友好的编辑对话框，false：指令仅支持 “显示所有值” 样式的编辑
/// category                string  Advanced    此指令所属的类别
/// paramRemove             string              覆盖时用于移除参数，例如：“1,3” 将在覆盖时移除参数 1 和 3
/// param[1-7]              object              MissionCommandParamInfo 对象
///
```

[任务参数信息https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandUIInfo.h#L25](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandUIInfo.h#L25):

```
/// MissionCommandParamInfo 用于为与 MAV_CMD 相关的参数自动生成编辑用户界面。
///
/// MissionCommandParamInfo is used to automatically generate editing ui for a parameter associated with a MAV_CMD.
///
/// MissionCmdParamInfo 对象的 JSON 格式如下：
///
/// 键              类型    默认值       描述
/// label           string  必填项      文本字段的标签
/// units           string              值的单位，应使用 FactMetaData 单位字符串以实现自动转换translation
/// default         double  0.0/NaN     参数的默认值。如果未指定默认值且 nanUnchanged == true，则 defaultValue 为 NaN。
/// decimalPlaces   int     7           值要显示的小数位数
/// enumStrings     string              组合框中显示供选择的字符串
/// enumValues      string              与每个枚举字符串关联的值
/// nanUnchanged    bool    false       True: 值可设置为 NaN 以表示未更改
```
