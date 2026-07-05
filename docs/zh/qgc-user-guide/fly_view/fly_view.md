# 飞行视图

:::tip Having trouble?
For connection issues, see [Connection Problems](../troubleshooting/vehicle_connection.md). For mission resume issues during flight, see [Resume Mission Failures](../troubleshooting/resume_mission.md).
:::

飞行视图用于指挥和监视载具。

## 概述

- **[工具栏](fly_view_toolbar.md):** 工具栏位于屏幕顶部。 它提供了选择视图的控制，显示飞行状态和方式以及车辆主要部件的状况。
- **[载具动作](fly_tools.md)：** 允许您命令车辆采取特定行动。
- **[仪器面板](instrument_panel.md)：** 显示载具遥测器的部件。
- **[姿态/罗盘](hud.md)：** 一个提供虚拟地平线和航向信息的部件。
- **[相机工具](camera_tools.md)**：用于在仍然和视频模式之间切换、开始/停止捕获以及控制相机设置的部件。
- **[视频](video.md):** 显示载具中的视频。 允许您在视频或地图之间切换为主显示。
- **地图：** 显示所有连接车辆的位置和当前车辆的任务。
  - 你可以拖动地图来移动它周围(地图在一定时间后自动重置在车上)。
  - 你可以使用缩放按钮、鼠标滚轮、触控板或在平板电脑上捏合操作来缩放地图。
  - Once flying, you can click on the map to set a [Go to](#map_actions) or [Orbit at](#map_actions) location.

还有一些其他元素没有默认显示并且只显示在某些条件下或某些类型的车辆。

## 操作/任务

以下各节介绍如何在Fly View中执行共同的操作/任务。

:::info
许多可用的选项都取决于车辆类型及其目前状态。
:::

### Actions associated with a map position {#map_actions}

可以采取一些与地图上的具体立场相关的行动。 要使用这些动作：

1. 在指定位置点击地图
2. 弹出窗口将显示您的可用动作列表
3. 选择您想要的操作
4. 确认操作

地图位置操作示例包括前往位置、环绕飞行等等。

### 暂停

您可以暂停大多数操作，包括起飞、着陆、返航、任务执行、轨道等。 暂停使用时的车辆行为取决于载具类型；通常是多层飞行器悬停，固定翼飞行器将环绕。

:::info
您不能暂停 _前往位置_ 操作。
:::

要暂停：

1. 按 _飞行工具_ 中的 **返航** 按钮。
2. 可选使用右侧垂直滑块设置新海拔。
3. [Confirm](fly_tools.md#confirmation) the action.

### 任务

#### 开始任务 {#start_mission}

You can start a mission when the vehicle is landed (the start mission confirmation button is often displayed by default).

要从着陆状态启动任务：

1. 按下 “飞行工具” 上的 _操作_ 按钮

2. 在对话框中选择  _开始任务_ 操作。

3. [Confirm](fly_tools.md#confirmation) the action to start the mission.

#### 继续任务{#continue_mission}

You can _continue_ mission from the _next_ waypoint when you're flying (the _Continue Mission_ confirmation button is often displayed by default after you takeoff).

:::info
继续和 [回复任务](#resume_mission) 是不一样的！
_继续_ 用于重新启动已暂停的任务，或者你已经起飞，因此已经错过起飞任务指令的情况。
恢复任务 用于在执行任务中途使用了返航或着陆（例如更换电池），然后希望继续执行下一个任务项目的情况（也就是说，它会将你带到任务中之前执行到的位置，而不是从任务中的当前位置继续）。
:::

你可以继续当前任务（除非已经在执行任务）：

1. 按下 _飞行工具_ 上的**操作**按钮

2. 在对话框中选择 _继续任务_ 操作。

3. [Confirm](fly_tools.md#confirmation) the action to continue the mission.

#### 回复任务{#resume_mission}

_Resume Mission_ is used to resume a mission after performing an [RTL/Return](hud.md#rtl) or [Land](hud.md#land) from within a mission (in order, for example, to perform a battery change).

:::info
如果你正在更换电池，断开电池后**请勿**断开 QGC 与飞行器的连接。
插入新电池后，“QGroundControl” 将再次检测到飞行器并自动恢复连接。
:::

着陆后，系统将弹出 _飞行计划完成” 对话框，你可以选择从飞行器中删除该计划、_将其保留在飞行器上，或者从最后经过的航点恢复任务。

如果你选择回复任务，_QGroundControl_ 会重新构建任务并将它上传到载具。
Then [confirm](fly_tools.md#confirmation) the action to continue the mission.

:::info
任务不能简单地从飞行器执行的最后一个任务项目继续，因为在最后一个航点可能有多个影响任务下一阶段的项目（例如速度指令或相机控制指令）。
相反，_QGroundControl_ 会从最后飞行的任务项目开始重构任务，并自动将任何相关指令添加到任务开头。
:::

#### 着陆后移除任务提示 {#resume_mission_prompt}

任务完成、飞行器着陆并锁定后，系统会提示你从飞行器中移除该任务。
这旨在防止陈旧的任务在不知不觉中留在飞行器上，从而可能导致意外行为的问题。

### 显示视频 {#video_switcher}

启用视频流传输后，_QGroundControl_ 会在地图左下角的 “视频切换窗口” 中显示当前选定飞行器的视频流。
You can press the switcher anywhere to toggle _Video_ and _Map_ to foreground.

:::info
视频流已配置/启用在 [应用程序设置 > 常规 > 视频](../settings_view/general.md#video)。
:::

您可以在开关上使用控件来进一步配置视频显示：

- 通过拖动右上角的图标来调整切换器的大小。
- 点击左下方的切换图标来隐藏切换器。
- 按下视频切换器窗口左上角的图标可将其分离（分离后，你可以像操作系统中的其他窗口一样移动和调整该窗口的大小）。
  如果您关闭分离的窗口，切换器将重新锁定到 QGC 飞行视图。

### 录制视频

如果相机和载具支持，_QGroundControl_ 可以在相机上开始和停止视频录制。 _QGroundControl_ 也可以录制视频流并在本地保存。

:::tip
存储在相机上的视频质量可能要高得多。 但你的地面站很可能有更大的录制能力。
:::

#### 录制视频流 (GCS)

Video stream recording is controlled on the [video stream instrument page](hud.md#video_instrument_page).
按下红色圆圈开始录制新视频(每次按下圆圈时创建新视频文件)； 正在录制时，圆圈将会变成红色正方形。

视频流录制已配置在 [应用程序设置 > 常规](../settings_view/general.md)：

- [视频录制](../settings_view/general.md#video-recording) - 指定录制文件格式和存储限制。

  ::: info
  视频默认保存为 Matroska 格式 (.mkv) 。
  这种格式在出现错误的情况下，相对不容易损坏。
  :::

- [杂项](../settings_view/general.md#miscellaneous) - 流视频保存在**应用程序加载/保存路径**下。

:::tip
存储的视频仅包括视频流本身。
要使用显示的 QGroundControl 应用程序元素录制视频，您应该使用单独的屏幕录制软件。
:::

#### 在相机上录制视频

Start/stop video recording _on the camera itself_ using the [camera instrument page](hud.md#camera_instrument_page).
首先切换到视频模式，然后选择红色按钮开始录制。

