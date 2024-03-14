# 用户界面控件

QGC提供了一组用于构建用户界面的基本控件。 一般来说，它们往往是Qt支持的基本QML控件上方的薄层，Qt控件支持QGC调色板。 In general they tend to be thin layers above the base QML Controls supported by Qt which respect the QGC color palette.

```
import QGroundControl.Controls 1.0
```

## Qt控件

以下控件是标准Qt QML控件的QGC变体。 除了使用QGC调色板绘制。它还们提供与相应Qt控件相同的功能， They provide the same functionality as the corresponding Qt controls except for the fact that they are drawn using the QGC palette.

- QGCButton
- QGCCheckBox
- QGCColoredImage
- QGCComboBox
- QGCFlickable
- QGCLabel
- QGCMovableItem
- QGCRadioButton
- QGCSlider
- QGCTextField

## QGC 控件

这些自定义控件是QGC独有的，用于创建标准UI元素。

- DropButton - RoundButton，单击时会删除一组选项。 示例是平面视图中的同步按钮。 Example is Sync button in Plan view.
- ExclusiveGroupItem - 用于支持QML ExclusiveGroup 概念的自定义控制的基础项目。
- QGCView - Base control for all top level views in the system. QGCView - 系统中所有顶级视图的基本控件。 提供对FactPanels的支持并显示QGCViewDialogs和QGCViewMessages。
- QGC View对话框 - 从QGC视图右侧弹出的对话框。 您可以指定对话框的接受/拒绝按钮以及对话框内容。 使用示例是当您单击某个参数并显示值编辑器对话框时。 You can specific the accept/reject buttons for the dialog as well as the dialog contents. Example usage is when you click on a parameter and it brings up the value editor dialog.
- QGCViewMessage - QGCViewDialog的简化版本，允许您指定按钮和简单的文本消息。
- QGCViewPanel - QGCView内部的主要视图内容。
- RoundButton - 一个圆形按钮控件，它使用图像作为其内部内容。
- SetupPage - 所有安装载具组件页面的基本控件。 提供标题，说明和组件页面内容区域。 Provides a title, description and component page contents area.
