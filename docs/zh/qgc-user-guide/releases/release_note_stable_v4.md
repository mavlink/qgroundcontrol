# QGC v4 版本说明

:::warning
现在在[Github 发布页面](https://github.com/mavlink/qgroundcontrol/releases)跟踪发布笔记。
v4.0.0后的更改信息请查看该页面。
:::

## 稳定版本 4.0

:::info
QGC 中设置的格式必须在此版本中更改。 这意味着所有 QGC 设置将被重置为默认值。
:::

- 设置
  - 语言：允许选择语言
  - 可选的[CSV 日志](../settings_view/csv.md)遥测数据以提高访问性。
  - ArduPilot
    - 支持可配置的 mavlink 流速率。 可从 Settings/Mavlink 页面获取。
      ![流码率JPG](../../../assets/daily_build_changes/arducopter_stream_rates.jpg)
    - 改进对 ChibiOS 固件刷入的支持
    - 对连接到 ChibiOS 引导加载程序板的支持得到改进。
- 配置
  - 手柄
    - 新摇杆设置 ui
    - 能够配置按住按钮进行单次或重复操作
  - ArduPilot
    - 电机测试
    - ArduSub
      - 电机方向自动检测
    - ArduCopter
      - PID 调节支持 ![PID Tuning JPG](../../../assets/daily_build_changes/arducopter_pid_tuning.jpg)
      - 附加基本调整选项 ![基本调整 JPG](../../../assets/daily_build_changes/arducopter_basic_tuning.jpg)
      - 多旋翼飞行器 / 地面车辆 - 机架设置用户界面 ![Setup Frame Copter/Rover - Frame setup ui ![Setup Frame Copter JPG](../../../assets/daily_build_changes/arducopter_setup_frame.jpg)
- Plan
  - Create Plan from template with wizard like progression for completing full Plan.
  - Survey: Save commonly used settings as a Preset
  - Polygon editing
    - New editing tools ui
    - Support for tracing a polygon from map locations
  - ArduPilot
    - Support for GeoFence and Rally Points using latest firmwares and mavlink v2
  - [Pattern Presets](../plan_view/pattern_presets.md)
    - Allows you to save settings for a Pattern item (Survey, Corridor Scan, ...) into a named preset. You can then use this preset over and over again as you create new Pattern.
- Fly
  - Click to ROI support
  - Added support for connecting to ADSB SBS server. 增添了对来自 USB 软件定义无线电（SDR）加密狗的ADSB数据的支持（例如 “dump1090 --net”）。
  - 能够在指南针中开启首向起飞点、对地航迹（COG）和下一个航点标题指示器。
  - 视频
    - 添加对 h.265 视频流的支持
    - 自动添加一个带有飞行数据的[视频重叠](../fly_view/video_overlay.md)作为本地录制视频的字幕
  - 飞行前的具体载具类型核对表。 从设置中打开
- 分析
  - New Mavlink Inspector which includes charting support. Supported on all builds including Android and iOS.
- General
  - Released Windows build are now 64 bit only
  - Log Replay: Ability to specify replay speed
  - ArduPilot
    - Improved support for chibios firmwares and ArduPilot bootloader with respect to flashing and auto-connect.
