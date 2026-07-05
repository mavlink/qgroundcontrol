# 视频

当视频流启用时(应用程序设置-视频)， _QGroundControl_ 将在地图底部的“视频切换窗口”中显示当前选中载具的视频流。
You can press the switcher anywhere to toggle _Video_ and _Map_ to foreground.

:::info
视频流已配置/启用在 [应用程序设置 > 常规 > 视频](../settings_view/general.md#video)。
:::

您可以在开关上使用控件来进一步配置视频显示：

- 通过拖动右上角的图标来调整切换器的大小。
- 点击左下方的切换图标来隐藏切换器。
- 按下视频切换器窗口左上角的图标，将其分离
  （分离后，你可以像操作操作系统中的其他窗口一样移动和调整该窗口的大小）。
  如果您关闭分离的窗口，切换器将重新锁定到 QGC 飞行视图。

### 录像

如果相机和载具支持，_QGroundControl_ 可以在相机上开始和停止视频录制。 _QGroundControl_ 也可以录制视频流并在本地保存。

:::tip
相机上存储的视频质量可能更高，但很可能你的地面站会有大得多的录制容量。
:::

#### 录制视频流 (GCS)

Video stream recording is controlled on the [video stream instrument page](hud.md#video_instrument_page).
按下红色圆圈开始录制新视频(每次按下圆圈时创建新视频文件)； 正在录制时，圆圈将会变成红色正方形。

视频流录制已配置在 [应用程序设置 > 常规](../settings_view/general.md)：

- [视频录制](../settings_view/general.md#video-recording) - 指定录制文件格式和存储限制。

  ::: 信息
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
先切换到视频模式，然后选择红色按钮开始录制。

