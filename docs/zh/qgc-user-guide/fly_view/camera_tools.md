# 相机工具

相机工具用于拍摄静态图像和视频，以及对相机进行配置。

![相机面板](../../../assets/fly/camera_panel/camera_mavlink.png)

相机捕获和配置选项取决于已连接的相机。
配置选项是使用面板装备图标选择的。
一个简单的自动化摄像头的配置显示在下面。

![相机面板 - 最小设置](../../../assets/fly/camera_panel/camera_settings_minimal.png)

当连接到支持[MAVLink 相机协议](https://mavlink.io/en/services/camera.html)的相机时，您可以另外配置和使用它提供的其他相机服务。
例如，如果您的摄像头支持视频模式，您将能够在仍然存在的图像捕获和视频模式之间切换，并且开始/停止录制。

![相机面板 - MAVLink 设置](../../../assets/fly/camera_panel/camera_settings_mavlink.png)

::: 信息
所显示的大多数设置都依赖摄像头(它们是在 [MAVLink 相机定义文件](https://mavlink.io/en/services/camera_def.html) 中定义的)。

> 末尾的一些常见设置是硬编码的：照片模式 (单张/时间线), 照片间隔(如果时间线)，重置相机默认值(将重置命令发送到相机)，格式(存储)
> :::

### 视频流 {#video_instrument_page}

视频页面用于启用/禁用视频流。
启用后，您可以开始/停止视频流，启用网格叠加层， 更改图像如何适合屏幕，并用QGC 在本地录制视频。

![仪表页面 - 视频流](../../../assets/fly/instrument_page_video_stream.jpg)