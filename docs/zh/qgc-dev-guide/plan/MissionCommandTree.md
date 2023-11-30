# 任务指令树

QGC 创建用户界面，用于从 json 元数据的层次结构中动态编辑特定任务项命令。 此层次结构称为任务命令树。 这样，在添加新命令时，只能创建 json 元数据。

## 为什么是一颗树？

需要该树以不同的方式处理不同固件和／或不同的车辆类型，以支持不同的命令。 最简单的例子是 mavlink 规范可能包含了并非所有固件都支持的命令参数。 或着命令参数仅对某些车辆类型有效。 此外，在某些情况下，GCS 可能会决定将某些命令参数在视图中对最终用户进行隐藏，因为它们过于复杂或导致可用性问题。

该树是MissionCommandTree类： [MissionCommandTree.cc](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandTree.cc), [MissionCommandTree.h](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandTree.h)

### 树根目录

树的根目录是与 mavlink 规范完全匹配的json元数据。

您可以在这里看到`MAV_CMD_NAV_WAYPOINT`根目录[json](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoCommon.json#L27)的示例：

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

注意：在现实中，基于此的信息应由 mavlink 本身提供，而不需要成为 GCS 的一部分。

### 叶节点

然后，叶节点提供元数据，这些元数据可以覆盖命令的值和/或从显示给用户的参数中删除参数。 完整的树层次结构是这样的：

- 根－通用Mavlink
  - 特定的车辆类型－特定于车辆的通用规范
  - 特定的硬件类型－每个固件类型有一个可选的叶节点（PX4/ArduPilot）
     - 特定的车辆类型－每个车辆类型有一个可选的叶节点（FW/MR/VTOL/Rover/Sub）

注意：实际上，此替代功能应该是mavlink规格的一部分，并且应该可以从车辆中查询。

### 从完整树中构建实例树

由于 json 元数据提供了所有固件／车辆类型组合的信息，因此必须根据用于创建计划的固件和车辆类型来构建要使用的实际树。 这是通过一个进程调用“collapsing”的完整树到一个固件／车辆的特定树来完成的 ([code](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandTree.cc#L119))。

步骤如下：
* 在实例树种添加根
* 将特定的车辆类型重写实例树
* Apply the firmware type specific overrides to the instance tree
* 将特定的硬件／车辆类型重写实例树

然后，生成的任务命令树将为平面项目编辑器构建UI。 实际上，它不仅用于此，还有许多其他地方可以帮助您了解有关特定命令 id 的更多信息。

## 层次结构示例 `MAV_CMD_NAV_WAYPOINT`

让我们来看看`MAV_CMD_NAV_WAYPOINT`的示例层次结构。 根信息如上图所示。

### 根－车辆类型的特定叶节点
层次结构的下一个层级是通用的 mavlink，但只针对特定的车辆。 这里的Json文件：[MR](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoMultiRotor.json), [FW](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoFixedWing.json), [ROVER](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoRover.json), [Sub](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoSub.json), [VTOL](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MavCmdInfoVTOL.json)。 这个是重写（固定翼）　

```
        {
            "id":           16,
            "comment":      "MAV_CMD_NAV_WAYPOINT",
            "paramRemove":  "4"
        },
```

这样做是删除参数4的编辑 UI，固定翼没有使用航向（Yaw）参数。 由于这是根的叶节点，因此无论固件类型如何，这都适用于所有固定翼车辆。

### 根－硬件类型的特定叶节点
层次结构的下一层级是特定于固件类型但适用于所有车辆类型的替代。  再次让我们看看航点（Waypoint）覆盖：

[ArduPilot](https://github.com/mavlink/qgroundcontrol/blob/master/src/FirmwarePlugin/APM/MavCmdInfoCommon.json#L6)：

```
        {
            "id":           16,
            "comment":      "MAV_CMD_NAV_WAYPOINT",
            "paramRemove":  "2"
        },
```

[PX4](https://github.com/mavlink/qgroundcontrol/blob/master/src/FirmwarePlugin/PX4/MavCmdInfoCommon.json#L7)：

```
        {
            "id":           16,
            "comment":      "MAV_CMD_NAV_WAYPOINT",
            "paramRemove":  "2,3"
        },
```

您可以看到，对于两个固件参数参数2，即接受半径，从编辑 ui 中删除。 这是QGC的特性决定。 与指定值相比，使用固件通用接受半径会更加安全和容易。 因此，我们决定对用户隐藏它。

您还可以看到，对于 PX4 param3/PassThru，由于 PX 不支持它，因此已被删除。

### 根－特定于固件的类型－特定于车辆类型的叶子节点
层次结构的最后一个级别既针对固件又针对车辆类型。

[ArduPilot/MR](https://github.com/mavlink/qgroundcontrol/blob/master/src/FirmwarePlugin/APM/MavCmdInfoMultiRotor.json#L7):

```
        {
            "id":           16,
            "comment":      "MAV_CMD_NAV_WAYPOINT",
            "paramRemove":  "2,3,4"
        },
```

在这里你可以看到，ArduPilot的多电机车辆参数2/3/4 Acceptance/PassThru/Yaw 已被移除。 例如，航向（Yaw）是因为不支持所以被移除。 由于此代码的工作原理的怪癖，您需要从较低级别重复重写。

## 任务命令 UI 信息
两个类定义与命令相关联的元数据：

* MissionCommandUIInfo－整个命令的元数据
* MissionCmdParamInfo－命令中参数的元数据

源中注释了支持 json 键的完整详细信息。

[MissionCommandUIInfo](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandUIInfo.h#L82)：

```
/// 与任务命令关联的 UI 信息 （MAV_CMD）
///
///MissionCommandUIInfo用于自动为MAV_CMD生成编辑ui。 此对象还支持仅具有一组命
/// 令的部分信息的概念。 这用于创建基本命令信息的替代。 对于覆盖，只需从基本命
/// 令 ui 信息中指定要修改的键即可。 若要重写 ui 参数信息，必须指定整个MissionParamInfo对象。
///
/// MissionCommandUIInfo对象的json格式为：
///
/// 键值                   　类型    默认值     描述
/// id                      int     reauired    MAV_CMD id
/// comment                 string              用于添加评论
/// rawName                 string  required    MAV_CMD 枚举名称，仅应设置基础树信息
/// friendlyName            string  rawName     命令的简单描述
/// description             string              命令的详细描述
/// specifiesCoordinate     bool    false       true: 命令指定一个纬／经／高坐标
/// specifiesAltitudeOnly   bool    false       true: 命令仅指定高度（非坐标）
/// standaloneCoordinate    bool    false       true: 车辆无法通过与命令关联的坐标（例如：ROI）
/// isLandCommand           bool    false       true: 命令指定着陆指令 (LAND, VTOL_LAND, ...)
/// friendlyEdit            bool    false       true: 命令支持友好的编辑对话框，false：Command仅支持“显示所有值”样式的编辑
/// category                string  Advanced    该命令所属的类别
/// paramRemove             string              由替代使用以删除参数，例如：“ 1,3”将删除替代上的参数1和3
/// param[1-7]              object              MissionCommandParamInfo 对象
///

```

[MissionCmdParamInfo](https://github.com/mavlink/qgroundcontrol/blob/master/src/MissionManager/MissionCommandUIInfo.h#L25)：

```
/// 与任务命令 （MAV_CMD） 参数关联的 UI 信息
///
/// MissionCommandParamInfo 用于自动为与 MAV_CMD 关联的参数生成编辑ui。
///
/// MissionCmdParamInfo 对象的 Json 文件格式为：
///　
/// 键值             类型    默认值     描述
/// label           string  required    文本字段标签
/// units           string              值的单位，应使用 FactMetaData Units 字符串以获得自动转换
/// default         double  0.0/NaN     默认参数值。 如果未指定默认值且 nanunchange==true，默认值为NaN。
/// decimalPlaces   int     7           显示值得小数位数
/// enumStrings     string              要在组合框中显示以供选择的字符串
/// enumValues      string              与每个枚举字符串关联的值
/// nanUnchanged    bool    false       True: 值可以设置为NaN表示信号不变
```
