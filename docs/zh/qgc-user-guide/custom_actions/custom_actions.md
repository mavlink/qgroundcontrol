# 自定义 Mavlink 动作

飞行视图和操纵杆都支持对正在运行的载具执行任意的MAVLink命令的功能。 在飞行视图中，这些将会显示在工具条行动列表中。 通过操纵杆，你可以将命令分配给按钮按下操作。

## Mavlink 动作文件

在 JSON 文件中定义了可用的动作。 该文件的格式如下：

```
{
    "version":    1,
    "fileType":   "MavlinkActions",
    "actions": [
        {
            "label":        "First Mavlink Command",
            "description":  "This is the first command",
            "mavCmd":       10,
            "compId":       100,
            "param1":       1,
            "param2":       2,
            ...
        },
        {
            "label":        "Second Mavlink Command",
            "description":  "This is the second command",
            "mavCmd":       20,
            ...
        }
    ]
}
```

字段：

- actions (必填) - 每个命令一个 json 对象数组
- label (必填) - 命令的用户可见简短描述。 这被用作飞行视图 - 操作命令列表中的按钮文本。 对于操纵杆，这是您从下拉菜单中选择的命令。 对于操纵杆，请确保您的名字与内置名称不冲突。
- description (必填) - 这是Fly View 中使用的命令的较长描述 - 动作列表。 操纵杆并不会使用这个字段。
- mavCmd (必填) - 你想要发送的 mavlink 命令的命令ID。
- compId (可选) - 你想要将命令发送到哪里的组件id。 如果未指定`MAV_COMP_ID_AUTOPILOT1`，则使用它。
- param1 到 param7 (可选) - 命令的参数。 未指定的参数将默认为0.0

Mavlink 操作文件应该位于QGC 保存位置的MavlinkAction目录。 例如，在 Linux 上，它是 "~/Documents/QGroundControl/MavlinkActions" 或 "~/Documents/QGroundControlDaily/MavlinkActions" 。 飞行视图和操纵杆都可以有自己的自定义操作文件。

当你启动QGC时，如果这些文件存在，它将加载这些文件，并使相关命令可供使用。
