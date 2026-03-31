# QGroundControl USV (无人船) 定制构建

## 概述

这是一个针对无人船 (Unmanned Surface Vehicle, USV) 的 QGroundControl 定制构建。

### 主要特性

- 🚤 **载具类型锁定**：仅支持 Rover/Boat 类型载具
- 🔧 **双固件支持**：同时兼容 ArduPilot (ArduRover) 和 PX4
- ✂️ **功能精简**：隐藏不适用于无人船的功能（起飞、降落、高度控制等）
- 📋 **专用检查清单**：无人船预航检查清单（含载荷状态自动检测）
- 🎨 **可选品牌定制**：支持自定义 Logo 和主题颜色
- 📊 **水质载荷面板**：实时显示检测电压、吸光度、泵组角度，支持采样/校准控制
- 🧭 **综合仪表盘**：替换默认罗盘，集成航行状态和姿态监测
- ⚠️ **姿态预警**：船体横滚/俯仰超限时顶部闪烁警告

## 支持的固件

| 固件 | 版本 | 状态 |
|------|------|------|
| ArduPilot (ArduRover) | 4.x+ | ✅ 完全支持 |
| PX4 | 1.14+ | ✅ 完全支持 |

用户可以在设置中选择使用 ArduPilot 或 PX4 固件进行离线任务规划。

## 构建方法

1. 确保 `custom` 目录位于 QGC 源码根目录
2. 清理之前的构建：
   ```bash
   rm -rf build
   ```
3. 重新配置和构建：
   ```bash
   cmake -B build -S .
   cmake --build build
   ```

## 目录结构

```
custom/
├── cmake/
│   └── CustomOverrides.cmake           # CMake 配置覆盖
├── res/
│   ├── img/
│   │   ├── usv-logo-white.png          # 白色 Logo (深色主题)
│   │   └── usv-logo-black.png          # 黑色 Logo (浅色主题)
│   ├── actions/
│   │   └── usv_actions.json            # MAVLink Actions 定义
│   ├── USVChecklist.qml                # 无人船预航检查清单 (qrc 覆盖)
│   ├── USVFlyViewCustomLayer.qml       # 飞行视图自定义层 (qrc 覆盖)
│   ├── USVFlyViewCustomLayer_TEST.qml  # 覆盖机制测试文件
│   ├── USVFlyViewBottomRightRowLayout.qml  # 隐藏原版遥测条 (qrc 覆盖)
│   ├── USVInstrumentPanel.qml          # 综合仪表盘 (qrc 覆盖 IntegratedCompassAttitude)
│   ├── USVPayloadPanel.qml             # 水质载荷控制面板 (USV QML 模块)
│   ├── USVPayloadFactGroup.json        # 载荷 FactGroup 元数据
│   ├── USVSelectViewDropdown.qml       # 中文化视图选择菜单 (qrc 覆盖)
│   └── USVToolBarButton.qml            # 主题感知 Logo 按钮 (qrc 覆盖)
├── src/
│   ├── USVPlugin.h/cc                  # 主插件 + URL 拦截器
│   ├── USVOptions.h/cc                 # 选项配置 (功能开关/检查清单)
│   ├── USVPayloadFactGroup.h/cc        # 水质载荷 FactGroup (MAVLink 数据)
│   ├── FirmwarePlugin/
│   │   ├── USVFirmwarePlugin.h/cc      # 双固件插件 (ArduPilot + PX4)
│   │   └── USVFirmwarePluginFactory.h/cc
│   └── AutoPilotPlugin/
│       └── USVAutoPilotPlugin.h/cc     # 双固件自动驾驶插件
├── CMakeLists.txt                      # 构建配置 + USV QML 模块定义
├── custom.qrc                          # qrc 覆盖资源映射
└── README.md
```

## 架构说明

### QML 覆盖机制

通过 `USVQmlOverrideInterceptor`（`QQmlAbstractUrlInterceptor`）实现 QML 文件替换：
- 拦截器检查 `:/USV` + 原始路径是否存在，存在则返回覆盖 URL
- 覆盖映射在 `custom.qrc` 中定义
- 例：`qrc:/qml/QGroundControl/FlyView/FlyViewCustomLayer.qml` → `qrc:/USV/qml/QGroundControl/FlyView/FlyViewCustomLayer.qml`

### USV QML 模块

通过 `qt_add_qml_module(USVModule URI USV VERSION 1.0)` 注册：
- `USVPayloadPanel.qml` 作为模块组件，通过 `import USV 1.0` 引用
- `addImportPath("qrc:/qml")` 使模块可被发现
- 注意：通过 qrc 覆盖加载的文件（Checklist、FlyViewCustomLayer 等）**不应**同时在 USV 模块的 QML_FILES 中注册，避免双重加载冲突

### 载荷数据流

```
Jetson Nano → NAMED_VALUE_FLOAT MAVLink → Vehicle → USVPayloadFactGroup.handleMessage()
                                                   → QML: vehicle.getFact("usvPayload.xxx")
```

- `USVPayloadFactGroup` 通过 FirmwarePlugin 的 `factGroups()` 注册到 Vehicle
- Vehicle 自动将 MAVLink 消息分发给所有注册的 FactGroup
- QML 通过 `vehicle.getFact("usvPayload.status")` 等访问数据

### QML Import 规范

custom QML 文件应遵循以下 import 风格：
```qml
import QtQuick
import QtQuick.Controls
import QGroundControl
import QGroundControl.Controls
```

**禁止使用**以下 import（当前构建中不可用，会导致组件加载失败和崩溃）：
- `import QGroundControl.ScreenTools`
- `import QGroundControl.Palette`
- `import QGroundControl.Vehicle`

`ScreenTools`、`QGCPalette`、`Vehicle` 等类型通过 `import QGroundControl` 全局暴露。

## 定制内容

### 1. 载具类型限制
- 离线编辑默认使用 Rover/Boat 类型
- 用户无法在设置中切换到其他载具类型（固定翼、多旋翼等）

### 2. 固件选择
- 默认使用 ArduPilot
- 用户可以在设置中切换到 PX4
- 两种固件都针对无人船进行了优化

### 3. 隐藏的功能
- 空速计校准
- 悬停/上升/下降速度设置
- 任务绝对高度显示
- Orbit 模式
- ROI 功能
- 固定翼/VTOL 降落模式
- VTOL 相关设置

### 4. 保留的功能
- 航点导航
- Survey 测量任务（水域测量）
- 走廊扫描（河道巡检）
- 返航 (RTL)
- 紧急停止
- 罗盘/加速度计/陀螺仪校准

### 5. 无人船专用检查清单
- 船体/螺旋桨/电池/GPS/罗盘/遥控器检查
- 解锁前电机/舵机/任务检查
- 下水前水域/天气/通信/载荷/安全检查
- 载荷状态自动检测（通过 `usvPayload.status` FactGroup）

### 6. 水质载荷控制面板 (USVPayloadPanel)
- 实时显示：检测电压、吸光度、泵组角度 (X/Y/Z/A)
- 控制按钮：开始采样、停止、暂停、恢复、零点校准
- 按钮根据载荷状态自动启用/禁用
- 状态指示灯带呼吸动画，故障状态红色高亮
- MAVLink 指令：CMD 31010-31014

### 7. 综合仪表盘 (USVInstrumentPanel)
- 替换默认 IntegratedCompassAttitude
- 集成罗盘、航行状态（航速/航向/油门/距Home）、姿态监测（横滚/俯仰）
- 姿态警告/危险双阈值，颜色渐变提示

### 8. 姿态危险警告横幅
- 横滚 > 25° 或俯仰 > 20° 时顶部闪烁红色警告
- 实时显示当前姿态角度

## 飞行模式对照

### ArduPilot (ArduRover)
| 模式 | 说明 |
|------|------|
| Manual | 手动控制 |
| Steering | 转向模式 |
| Hold | 定点保持 |
| Loiter | GPS 定点 |
| Auto | 自动任务 |
| RTL | 返航 |
| SmartRTL | 智能返航 |
| Guided | 引导模式 |

### PX4
| 模式 | 说明 |
|------|------|
| Manual | 手动控制 |
| Stabilized | 稳定模式 |
| Position | 位置模式 |
| Hold | 定点保持 |
| Mission | 任务模式 |
| Return | 返航 |
| Offboard | 外部控制 |

## 回滚方法

如需恢复原版 QGC：

```bash
# 重命名 custom 目录
mv custom custom-backup

# 或删除
rm -rf custom

# 重新构建
cmake -B build -S .
cmake --build build
```

## 自定义品牌

1. 准备 Logo 图片（SVG 格式推荐）
2. 替换 `res/img/` 目录中的文件
3. 重新构建

## 许可证

遵循 QGroundControl 原始许可证 (Apache 2.0 / GPLv3)。
