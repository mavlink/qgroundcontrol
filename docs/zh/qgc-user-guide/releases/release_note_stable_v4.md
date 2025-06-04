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
- 计划
  - 使用向导创建模板的计划，如完成完整计划的进度。
  - 调查：将常用设置保存为预设
  - 多边形编辑
    - 新的编辑工具 ui
    - 支持从地图位置追踪多边形
  - ArduPilot
    - 使用最新固件和mavlink v2支持GeoFence和集成点
  - [模式预设](../plan_view/pattern_presets.md)
    - 允许您保存图案项 (Survey, Corridor Scan, ...) 的设置 进入指定的预设。 在您创建新模式时，您可以连续多次使用这个预设。
- 飞行
  - 单击以支持 ROI
  - 添加支持连接到 ADSB SBS 服务器。 增添了对来自 USB 软件定义无线电（SDR）加密狗的ADSB数据的支持（例如 “dump1090 --net”）。
  - 能够在指南针中开启首向起飞点、对地航迹（COG）和下一个航点标题指示器。
  - 视频
    - 添加对 h.265 视频流的支持
    - 自动添加一个带有飞行数据的[视频重叠](../fly_view/video_overlay.md)作为本地录制视频的字幕
  - 飞行前的具体载具类型核对表。 从设置中打开
- 分析
  - 支持图表功能的新型MAVLink检查器。 支持所有构建，包括安卓和iOS。
- 基本配置
  - 已发布的 Windows 构建现在是64 位
  - 日志重播：指定重放速度的能力
  - ArduPilot
    - 在刷新和自动连接方面，改进对chibios firmwares和ArduPilot bootloader的支持。
