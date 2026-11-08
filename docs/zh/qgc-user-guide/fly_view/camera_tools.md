# 相机工具

相机工具用于拍摄静态图像和视频，以及对相机进行配置。

相机捕获和配置选项取决于已连接的相机。配置选项是使用面板装备图标选择的。一个简单的自动化摄像头的配置显示在下面。

当连接到支持[MAVLink 相机协议](https://mavlink.io/en/services/camera.html)的相机时，您可以另外配置和使用它提供的其他相机服务。例如，如果您的摄像头支持视频模式，您将能够在仍然存在的图像捕获和视频模式之间切换，并且开始/停止录制。

::: 信息
所显示的大多数设置都依赖摄像头(它们是在 [MAVLink 相机定义文件](https://mavlink.io/en/services/camera_def.html) 中定义的)。

> A few common settings at the end are hard-coded: Photo Mode (Single/Time Lapse), Photo Interval (if Time Lapse), Reset Camera Defaults (sends a reset command to the camera), Format (storage), and for a camera with a video stream, Grid Lines, Telemetry Overlay and Display Fit (under Video Display)
> :::

## 视频流 {#video_instrument_page}

视频页面用于启用/禁用视频流。
When enabled, you can start/stop the video stream, enable a grid overlay or the [live telemetry overlay](video_overlay.md#live-overlay), change how the image fits the screen, and record the video locally with QGC.

A single photo the camera can't take itself (it can't capture images, or not in video mode) is saved by QGC instead,
from the video on display, to the `Photo` folder of the
[Application Load/Save Path](../settings_view/general.md#load_save_path). The JPEG records when it was taken and where
the vehicle was (EXIF date and GPS position).
