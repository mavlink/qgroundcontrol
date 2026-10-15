# QGC 架构与模块地图

本文是 `src/` 的导览与关键数据流说明。改动代码前先定位所属层与模块，不要跨层直接依赖。

## 分层总览

```text
QML 视图层      FlyView / PlanView / AnalyzeView / MainWindow / Toolbar / FirstRunPromptDialogs
QML 基础控件    QmlControls / FlightMap / QtLocationPlugin
C++ 应用层      Vehicle / MissionManager / Camera / Gimbal / GPS / Joystick / FollowMe /
                LogManager / Terrain / PositionManager / VideoManager / ADSB / Viewer3D
领域抽象层      FirmwarePlugin(APM/PX4) / AutoPilotPlugins(APM/PX4) / FactSystem / API
通信协议层      Comms(Serial/UDP/TCP/Bluetooth/MockLink/LogReplay) / MAVLink
基础设施        Settings / AppSettings / Utilities(Geo,Math) / QGCApplication
```

## 模块清单

### 通信与协议

| 模块 | 职责 | 关键类型 |
| --- | --- | --- |
| `src/Comms` | 链路生命周期与收发：串口/UDP/TCP/蓝牙、日志回放、仿真链路 | `LinkManager`、`LinkInterface`、`SerialLink`、`UDPLink`、`TCPLink`、`MAVLinkProtocol` |
| `src/Comms/MockLink` | 无硬件仿真 MAVLink 链路（含相机/云台/FTP worker），测试与开发必备 | `MockLink`、`MockConfiguration` |
| `src/Comms/Bluetooth` | 蓝牙/BLE 链路 | `BluetoothLink` |
| `src/MAVLink` | MAVLink 封装：FTP、签名、LibEvents、状态文本、图片协议、流配置 | `QGCMAVLink`、`MAVLinkFTP`、`StatusTextHandler`、`MAVLinkSigning` |
| `src/ADSB` | ADS-B 交通目标接入（TCP）与车辆管理 | `ADSBVehicleManager`、`ADSBVehicle` |

### 核心模型

| 模块 | 职责 | 关键类型 |
| --- | --- | --- |
| `src/Vehicle` | 载具核心模型：指令队列、标准模式、FTP、混控执行器、Autotune、RemoteID | `Vehicle`、`VehicleTypes.h`、`StandardModes`、`FTPManager` |
| `src/FactSystem` | 参数系统基石；所有参数唯一入口 | `Fact`、`FactGroup`、`FactMetaData`、`ParameterManager`、`SettingsFact` |
| `src/FirmwarePlugin` | 固件差异抽象（APM/PX4 子目录） | `FirmwarePlugin`、`ParameterMetaData` |
| `src/AutoPilotPlugins` | "载具设置"页 UI 的固件差异（机架、传感器、校准等组件） | `AutoPilotPlugin`、`VehicleComponent` |
| `src/MissionManager` | 任务/航点/航测/走廊扫描/地理围栏/集结点的数据模型与控制器 | `MissionController`、`SimpleMissionItem`、`ComplexMissionItem`、`SurveyComplexItem`、`GeoFenceManager`、`PlanManager`、`MissionCommandTree` |
| `src/API` | 定制构建（OEM）扩展点 | `QGCCorePlugin`、`QGCOptions`、`QmlComponentInfo` |

### 外设与功能服务

| 模块 | 职责 |
| --- | --- |
| `src/Camera` | 相机控制、拍照/录像、视频流信息、模拟相机 |
| `src/Gimbal` | 云台控制与姿态稳定 |
| `src/GPS` | RTK GPS：`Driver/` 驱动、`NTRIP/` 差分客户端与源表、`RTCM/` 电文解析与 UDP 转发 |
| `src/Joystick` | 手柄/摇杆（SDL 后端）与虚拟摇杆支持 |
| `src/FollowMe` | GCS 位置上报、载具跟随 |
| `src/PositionManager` | 本机定位（含模拟位置源） |
| `src/VideoManager` | GStreamer 视频管道与视频设置 |
| `src/LogManager` | 飞行日志下载、列表模型、格式化 |
| `src/Terrain` | 地形高程查询与瓦片缓存（任务地形高度） |
| `src/Viewer3D` | 3D 视图 |

### UI（QML）

| 模块 | 职责 | QML URI |
| --- | --- | --- |
| `src/MainWindow` | 主窗口与保存的界面状态 | `QGroundControl` |
| `src/FlyView` | 飞行界面：地图/视频叠加、引导动作、飞行前检查表、虚拟摇杆、告警 | `QGroundControl.FlyView` |
| `src/PlanView` | 任务规划界面：航点/航测/围栏/集结点编辑器与地图图元 | `QGroundControl.PlanView` |
| `src/AnalyzeView` | 分析页：日志下载/查看、GeoTag 照片定位 | `QGroundControl.AnalyzeView` |
| `src/FlightMap` | 地图控件、比例尺、罗盘、地图图元 | — |
| `src/QtLocationPlugin` | 自研地图引擎、瓦片任务、离线缓存、URL 源 | — |
| `src/QmlControls` | 全应用复用控件与 C++ 图元（圆/多边形/折线、参数编辑器、仪表值等） | `QGroundControl.Controls` |
| `src/AppSettings` | 应用设置页 QML（链接、串口、UDP/TCP、离线地图、MockLink 等） | — |
| `src/Toolbar` | 顶部工具栏指示器（解锁、GPS、模式、电量等） | — |
| `src/FirstRunPromptDialogs` | 首次运行引导弹窗 | — |

### 基础设施

| 模块 | 职责 |
| --- | --- |
| `src/Settings` | 持久化设置：`SettingsGroup` 子类 + `*.SettingsGroup.json`（App/Mavlink/Maps/Video/NTRIP/RTK/Units/Joy 等） |
| `src/Utilities` | `Geo/`（地理几何）、`Math/`（数学工具）、`AppMessages` |
| `src/Android` | Android 平台初始化、串口、事件 |
| 应用入口 | `src/main.cc` → `src/QGCApplication`；预编译头 `src/pch.h` |

## 关键数据流

### 1. 链路到载具

```text
LinkManager(Comms) 创建/管理 LinkInterface 实现
  → SerialLink / UDPLink / TCPLink / BluetoothLink / MockLink / LogReplayLink
MAVLinkProtocol 解析 MAVLink 字节流（QGCMAVLink / LibEvents / StatusTextHandler）
  → Vehicle 模块按 sysid/compid 建立并维护 Vehicle
MultiVehicleManager 持有多机列表，activeVehicle() 是当前活动机
```

- 链路配置类派生 `LinkConfiguration`；新链路类型同时要在 AppSettings 提供配置页。
- 开发无硬件时用 MockLink（`MockConfiguration` 可指定 PX4/APM 风味）。

### 2. 参数流（Fact System）

```text
MAVLink 参数协议 → ParameterManager(Vehicle 内)
  → Fact（单个参数：rawValue 裸值 / cookedValue 工程单位）
  → FactGroup（成组参数，如 GPSRTKFactGroup、Gimbal）
  → FactMetaData（范围/枚举/单位，来自 *.json 与固件元数据）
Settings/SettingsFact 用于 GCS 自身持久化设置（非载具参数）
```

- 参数未同步完前 `parametersReady` 为 false；QML 中必须判空/判 ready。
- UI 控件直接绑 Fact 的 `rawValue`/`cookedValue`，不要在 QML/JS 里复制参数状态。

### 3. 任务规划

```text
PlanView(QML 编辑) ←→ MissionController
  → PlanManager（上传/下载/同步）→ MAVLink mission 协议
MissionItem 树：SimpleMissionItem（单航点）/ ComplexMissionItem
  （Survey、CorridorScan、StructureScan、LandingPattern）
Section（CameraSection / SpeedSection）以混入方式给航点加附加语义
GeoFenceManager / RallyPointManager 分别管围栏与集结点
地形高度由 Terrain 模块查询补齐；KML 导出在 KMLPlanDomDocument
```

### 4. 固件差异分发

```text
Vehicle::firmwarePlugin() → FirmwarePlugin 接口
  ├─ APMFirmwarePlugin (src/FirmwarePlugin/APM)
  └─ PX4FirmwarePlugin  (src/FirmwarePlugin/PX4)
载具设置 UI：AutoPilotPlugin → VehicleComponent 列表（APM/PX4 各自实现）
```

调用方永远面向 `FirmwarePlugin` 接口；新增固件行为 = 加虚函数 + 两边实现，而非在业务代码里判断固件类型。

### 5. C++ ↔ QML 边界

- 每个视图模块是独立 static lib + `qt_add_qml_module`（URI 如 `QGroundControl.FlyView`，`NO_PLUGIN`，`RESOURCE_PREFIX /qml`）。
- 根模块 `QGroundControlModule` URI 为 `QGroundControl`（见 `src/CMakeLists.txt`）；控件模块 URI 为 `QGroundControl.Controls`。
- C++ 类型在其所属 target 中用 `QML_ELEMENT` 等宏注册；QML 文件必须列进所在模块 `QML_FILES`，新增的图片/资源走 `qt_add_resources`。

## 优先阅读的关键文件

1. `src/FactSystem/Fact.h` — 参数系统基础
2. `src/Vehicle/Vehicle.h` — 载具核心模型
3. `src/FirmwarePlugin/FirmwarePlugin.h` — 固件抽象
4. `src/Comms/LinkManager.h` — 链路管理
5. `src/MissionManager/MissionController.h` — 任务控制
6. `src/QGCApplication.h` 与 `src/main.cc` — 启动与装配
7. `src/CMakeLists.txt` — 模块装配与根 QML 模块
