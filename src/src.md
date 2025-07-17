├── ADSB/                      — 📡   空中交通监视（接收和展示 ADS‑B 目标）
├── API/                       — 🌐   外部/内部 API 接口封装（HTTP、插件接口等）
├── AnalyzeView/               — 📊   日志和飞行数据分析界面（QML + C++ 支持）
├── Android/                   — 🤖   Android 平台相关代码（打包、权限、JNI 调用）
├── AutoPilotPlugins/          — 🚁   飞控参数页和组件（PX4/APM 等具体实现）
├── Camera/                    — 🎥   相机控制与图像抓取接口
├── Comms/                     — 🔗   通信管理（链路抽象、重连策略等高层逻辑）
├── FactSystem/                — ⚙️ 参数系统核心（Fact、FactMetaData、持久化）
├── FirmwarePlugin/            — 🛩️ 飞控行为抽象层（模式、命令、限幅等逻辑）
├── FlightDisplay/             — 🗺️ 飞行主界面（地图、HUD、遥控器状态 QML）
├── FlightMap/                 — 🌍   地图渲染与图层管理（航线、轨迹、POI 等）
├── FollowMe/                  — 🏃  “跟随我” 功能（基于移动设备位置控制无人机）
├── GPS/                       — 📍   GPS 数据解码与传感器融合
├── Gimbal/                    — 🎛️ 云台控制（MAVLink 云台消息封装与 UI 绑定）
├── Joystick/                  — 🎮   手柄/遥控器输入支持（多设备热插拔管理）
├── MAVLink/                   — 💬   MAVLink 协议实现与消息定义（自动生成的 .h + 解析器）
├── MissionManager/            — 🗺️ 航线任务管理（创建、编辑、上传、下载）
├── PlanView/                  — ✏️ 任务规划界面（QML 地图上绘制航点和任务类型）
├── PositionManager/           — 📌   位置源管理（GPS、模拟器、外部定位等切换）
├── QGCApplication.cc/.h       — 🚀   应用生命周期 & 启动流程入口
├── QmlControls/               — 🔧   可复用 QML 控件库（按钮、面板、状态框等）
├── QtLocationPlugin/          — 🗺️ QtLocation 地图后端插件（高德、谷歌、离线等）
├── RunGuard.cc/.h             — 🔒   单实例运行保护（防止多开）
├── Settings/                  — ⚙️ 应用及飞控设置项管理（UI 绑定 + 持久化）
├── Terrain/                   — ⛰️ 地形数据支持（DEM 加载与高度查询）
├── TestPlugin/                — 🧪   测试插件框架（用于内部或第三方功能验证）
├── UI/                        — 🖥️ 传统 Qt Widgets 界面代码（少量历史遗留）
├── UTMSP/                     — 📡   联合战术空域管理协议支持（U‑TM, UAS 通信）
├── Utilities/                 — 🛠️ 通用工具函数（日志、文件操作、数学、串口辅助等）
├── Vehicle/                   — 🚁   Vehicle 类及子组件（状态管理、命令发送、心跳处理）
├── VideoManager/              — 📹   视频流管理（RTSP/UDP/RTP 接收与渲染）
└── Viewer3D/                  — 🖼️ 3D 可视化（仿真、地形叠加、航线预览）


7 个关键路径
作用	目录/文件
MAVLink 消息处理	src/Vehicle/Vehicle.cc
UI 控制按钮	src/QmlControls/CustomControlView.qml
云台/舵机/起飞命令	Vehicle::Q_INVOKABLE 方法 + QML按钮绑定
飞控模式适配	src/FirmwarePlugin/GenericFirmwarePlugin.cc 或创建 VendorFirmwarePlugin
自定义显示变量	Q_PROPERTY + emit + QML 绑定
协议支持	src/mavlink/custom/ + CMakeLists.txt
航点任务同步	src/MissionManager/PlanManager.cc


1. src/Vehicle/
核心设备模型逻辑文件	用途
Vehicle.cc/h	✅ MAVLink 消息处理入口、自定义字段、控制指令封装
VehicleLinkManager.cc/h	管理串口/UDP 链路心跳状态（一般不改）
VehicleBatteryFactGroup.cc/h	电池信息分组（你可以仿照新增 VendorFactGroup）
ADSBVehicleManager.cc/h	空中广播目标管理（可选）


2. src/FirmwarePlugin/
飞控插件（飞行模式/指令行为封装）
文件	用途
GenericFirmwarePlugin.cc/h	✅ 默认插件，厂商适配建议从这里继承
FirmwarePlugin.cc/h	插件基类
FirmwarePluginManager.cc/h	插件注册与切换
🔧 建议创建：VendorFirmwarePlugin.cc/h	你自己的插件逻辑（自定义飞行模式/起飞逻辑等）


3. src/MissionManager/
任务规划与同步模块
文件	用途
PlanManager.cc/h	✅ 航点列表上传下载（支持标准 MAVLink 任务同步）
MissionController.cc/h	QML 控制器接口（Plan 页面）
SimpleMissionItem.cc/h	单个任务点模型（可以扩展自定义参数）


4. src/comm/
链路通信模块（串口、UDP、TCP）
文件	用途
SerialLink.cc/h	✅ 串口通信（你接飞控最常用）
UDPLink.cc/h	UDP 连接（仿真常用）
LinkInterface.cc/h	通用链路接口基类
LinkManager.cc/h	管理所有链路


5. src/QmlControls/
QML UI 可复用控件
文件	用途
CustomControlView.qml	✅ 你定制功能 UI 的主入口（Fly 页面）
InstrumentValue.qml	仪表控件（显示数值）
FactValueSlider.qml	滑块控件（如舵机 PWM 控制）

6. src/Settings/ 
参数设置、界面设置相关
文件	用途
AppSettings.cc/h	应用级配置项（如单位、UI风格）
SettingsManager.cc/h	设置项管理
✅ 可添加 VendorSettings.cc/h	你的厂商专属配置项（如波特率、协议版本）


📁 7. src/FactSystem/
Fact 数据绑定系统（用于 UI 与后台变量交互）
文件	用途
Fact.cc/h	值对象
FactGroup.cc/h	多个 Fact 的分组（如电池组）
✅ 可添加 VendorFactGroup.cc/h	展示厂商自定义数据（如健康、电压、状态码）


📁 8. src/MAVLink/
协议管理层
文件	用途
MAVLinkProtocol.cc/h	✅ 消息解析入口（调用 mavlink_parse_char）
QGCMAVLink.cc/h	MAVLink 消息辅助处理
MAVLinkMessages.proto	MAVLink 数据封装模型（用于 protobuf）

📁 9. src/VideoStreaming/（如果需要视频）
文件	用途
VideoReceiver.cc/h	支持 RTSP/UDP 流
VideoSurface.cc/h	渲染视频帧到 QML
VideoSettings.cc/h	视频设置界面控制器

📁 10. src/QtLocationPlugin/（地图功能）
文件	用途
QGeoTileFetcherQGC.cc	地图瓦片下载
QGeoMapReplyQGC.cc	HTTP 请求响应封装
QGCMapEngine.cc	地图缓存管理

📁 11. src/Airmap/（可选，空域服务）
文件	用途
AirspaceManager.cc/h	空域限制管理
AirspaceRestrictionProvider.cc/h	限飞区渲染支持

📁 12. src/mavlink/custom/your_vendor/ ✅ （你自定义的 MAVLink 消息头文件放这里）
内容	如何生成
mavlink_msg_vendor_status.h	使用 pymavlink 从你的 XML 文件生成
设置编译路径	set(QGC_MAVLINK_DIALECT your_vendor) in CMakeLists.txt

✅ 一、协议分析与开发工作拆解
1. 通信机制
   默认启动为 MAVLink 1.0，需地面站先发送 2.0 心跳包使其切换协议版本。

通信支持 UDP 和 TCP，建议确认你当前是串口、UDP、还是通过转发桥接。

✅ 你需要：

在 QGC 中设置使用 MAVLink 2.0，并修改心跳包发送格式。

配置 QGC 的通信方式，确保能建立稳定连接。

2. 控制类命令（已自定义实现）
   控制通过标准 COMMAND_LONG (ID=76) 实现，如：

一键解锁（command=400）

一键起飞（command=24，param7 为起飞高度）

一键返航（command=20）

一键设置模式（command=176 + 自定义 mode）

舵机控制（command=183）

云台控制（command=203，带复杂参数逻辑）

✅ 你需要：

在 Vehicle::_sendMavCommand 或 QGC Command 实现中添加上述 command 支持；

实现云台控制 UI 并正确构造相关参数；

若使用 CustomCommandWidget.qml 显示按钮，确保绑定的 command、param 设置正确。

3. 状态数据支持良好
   飞控主动上报：

姿态（30）、位置（33）、GPS（24/124）、电压（147）、航点状态（42）

解锁/飞行模式状态在 HEARTBEAT（0）、EXTENDED_SYS_STATE（245）

✅ 你需要：

在 Vehicle 类中提取这些数据，或通过 FactGroup 自动更新；

UI 上展示飞行模式、电池电压、GPS 等数据，需检查 QGC 是否已有对应字段映射。

4. 航点任务支持完整
   支持 Mission 上传、下载、ACK 等全流程（ID 44, 45, 51, 73 等）

支持航点指令（16）、返航（20）、任务起飞（22）、设置速度（178）等标准指令

✅ 你需要：

如果沿用 QGC 的任务编辑器（PlanView），可能仅需确保 command 映射和参数一致；

若自定义任务上传流程，需按协议流程构造任务同步逻辑。

5. 虚拟摇杆（手动控制）
   支持 MANUAL_CONTROL（69），要求使用 MAVLink2.0，包含拓展字段。

✅ 你需要：

判断是否使用 QGC 的 Joystick 功能，或自定义发送虚拟摇杆数据；

构造完整字段，尤其是 enabled_extensions=0x80，默认值需严格一致。

6. 串口透传（DATA32/64/96）未实现
   协议说明“待开发”，不建议使用，除非厂家已明确支持。

7. JSON 协议用于状态上报
   地面站本地可通过 JSON 读取状态，但不可通过 JSON 控制。

✅ 建议仅用于状态中转或 WebSocket 接口展示，不作控制使用。