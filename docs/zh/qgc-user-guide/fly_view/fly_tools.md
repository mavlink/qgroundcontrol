# 飞行工具

## Action Confirmation {#confirmation}

Safety-critical actions (takeoff, land, RTL, etc.) require confirmation before they are executed. When you trigger one of these actions, a confirmation button appears with chevrons on its left side. To confirm, click and hold the button until the action is accepted. This prevents accidental activation of critical commands.

## 飞行检查清单 {#preflight_checklist}

飞行前自动核对表可以用来进行标准检查，检查车辆配置正确，可以安全飞行。

To view the checklist, first enable it by navigating to [Application Settings > General > Fly View](../settings_view/general.md) and selecting the **Use preflight checklist** checkbox.
The checklist is then available on the **Pre-Flight Checklist** tab of the main status dropdown (click the status text at the left of the toolbar).
The dropdown opens on this tab until the checklist has passed.

完成每项测试后，在用户界面上选择该项，将其标记为已完成。

All checklist sections are shown at once. A section is locked (marked **(locked)**) until every section above it has passed, but its items still show their current state, so problems detected automatically — such as muted audio or too few GPS satellites — are visible from the start.
Items are colored by state: red for a failed check that must be fixed, orange for a check that still needs confirming, and green for a passed check.
The tab shows **Passed** once every check has passed, otherwise **In Progress**.

## 关闭 {#takeoff}

:::tip
如果你正在为多功能飞行器启动任务，_QGroundControl_ 将自动执行起飞步骤。
:::

起飞(降落时)：

1. 按 _Fly Tools_ 中的 **起飞** 按钮(这将在起飞后切换到 **着陆** 按钮)。
2. 在右侧垂直滑块中可选择设置起飞高度。

- 您可以向上/向下滑动以更改高度
- 您也可以点击指定的高度(例如10英尺)，然后输入特定的海拔。

1. [Confirm](#confirmation) the action.

## 着陆 {#land}

你可以在飞行时随时降落在当前位置：

1. 按下 _飞行工具_ 中的 **着陆** 按钮(当降落将切换到 **起飞** 按钮)。
2. [Confirm](#confirmation) the action.

## 返航/返回

飞行时返回“安全点”：

1. 按 _飞行工具_ 中的 **返航** 按钮。
2. [Confirm](#confirmation) the action.

:::info
飞行器通常返回 “Home点”（起飞）位置并着陆。这种行为取决于载具类型和配置。例如，集结点或飞行任务着陆可能被用作替代返回目标。
:::

## 更改高度 {#change_altitude}

飞行过程中你可以改变高度，但执行任务时除外：

1. 按 _飞行工具_ 上的 **操作** 按钮
2. 选择 _改变高度_ 按钮
3. 从垂直滑块选择新高度
4. 确认操作
