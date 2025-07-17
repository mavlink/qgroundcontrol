Comms/	链路管理 + MAVLinkProtocol（位于 Comms/MAVLinkProtocol/）
MAVLink/	MAVLink 协议头文件
Vehicle/	Vehicle 核心类及子组件
FirmwarePlugin/	飞控适配插件
AutoPilotPlugins/	飞控参数页面 & 组件
MissionManager/	航线任务管理
PlanView/	任务规划 QML
FlightDisplay/	飞行主界面 QML
FlightMap/	地图 & 轨迹显示
FactSystem/	参数系统核心
QmlControls/	通用 QML 控件
Settings/	应用 & 飞控设置
AppMessages*	全局弹窗 & 告警提示（在 FirstRunPromptDialogs/ 可合并或在 QGCApplication 内）
Audio*	（若在 Utilities/ 中实现，需保留提示音逻辑）
PositionManager/	GPS / 定位管理
Utilities/	日志、文件、数学等通用工具


可选
Gimbal/	如需云台控制保留
VideoManager/	如需视频监控保留
Joystick/	需手柄远程操控保留
Terrain/	需地形高度支持保留


删除
ADSB/	ADS‑B 空管监视，非必须
API/	Web/API 接口
AnalyzeView/	日志 & 数据分析界面
Android/	移动端专用
Camera/	相机控制
FollowMe/	“跟随我”功能
GPS/	底层 GPS 解码（已由 PositionManager 替代）
QtLocationPlugin/	非默认地图引擎插件
RunGuard.cc/.h	单实例保护（可用 OS 手段代替）
TestPlugin/	测试框架
UI/	旧版 QtWidgets 界面
UTMSP/	无人机通管理协议
Viewer3D/	3D 可视化