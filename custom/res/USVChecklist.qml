/****************************************************************************
 *
 * USV Pre-Flight Checklist - 无人船预航检查清单
 *
 * 专为无人船设计的检查清单，包含水上航行特有的检查项
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQml.Models

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView

Item {
    property var model: listModel
    PreFlightCheckModel {
    id: listModel

    // ========== 第一组：无人船初始检查 ==========
    PreFlightCheckGroup {
        name: qsTr("无人船初始检查")

        // 船体检查
        PreFlightCheckButton {
            name:       qsTr("船体")
            manualText: qsTr("船体密封完好？无破损或裂缝？")
        }

        // 螺旋桨检查
        PreFlightCheckButton {
            name:       qsTr("螺旋桨")
            manualText: qsTr("螺旋桨无缠绕物？叶片完好？")
        }

        // 电池检查
        PreFlightCheckButton {
            name:               qsTr("电池")
            telemetryFailure:   !activeVehicle || activeVehicle.battery.percentRemaining.value < 50
            telemetryTextFailure: qsTr("电量低于 50%")
            manualText:         qsTr("电池安装牢固？电量充足？")
        }

        // GPS 检查
        PreFlightCheckButton {
            name:               qsTr("GPS")
            telemetryFailure:   !activeVehicle || activeVehicle.gps.lock.value < 3
            telemetryTextFailure: qsTr("GPS 未锁定 (需要 3D Fix)")
            manualText:         qsTr("GPS 天线无遮挡？")
        }

        // 罗盘检查
        PreFlightCheckButton {
            name:               qsTr("罗盘")
            telemetryFailure:   activeVehicle ? (activeVehicle.sensorsUnhealthyBits & Vehicle.SysStatusSensor3dMag) : false
            telemetryTextFailure: qsTr("罗盘传感器异常")
            manualText:         qsTr("远离磁性干扰源？")
        }

        // 遥控器检查
        PreFlightCheckButton {
            name:               qsTr("遥控器")
            telemetryFailure:   activeVehicle ? activeVehicle.rcRSSI < 50 : true
            telemetryTextFailure: qsTr("遥控器信号弱")
            manualText:         qsTr("遥控器已开启？信号正常？")
        }
    }

    // ========== 第二组：解锁前检查 ==========
    PreFlightCheckGroup {
        name: qsTr("解锁前检查")

        // 解锁提示
        PreFlightCheckButton {
            name:       qsTr("解锁")
            manualText: qsTr("请在此处解锁无人船")
        }

        // 电机测试
        PreFlightCheckButton {
            name:       qsTr("电机")
            manualText: qsTr("螺旋桨周围无障碍物？轻推油门测试电机响应？")
        }

        // 舵机测试
        PreFlightCheckButton {
            name:       qsTr("舵机")
            manualText: qsTr("转向舵机响应正常？方向正确？")
        }

        // 任务检查
        PreFlightCheckButton {
            name:       qsTr("任务")
            manualText: qsTr("任务航点有效？航线无障碍物？")
        }
    }

    // ========== 第三组：下水前最后检查 ==========
    PreFlightCheckGroup {
        name: qsTr("下水前最后检查")

        // 水域检查
        PreFlightCheckButton {
            name:       qsTr("水域")
            manualText: qsTr("水域无障碍物？水深足够？无渔网/水草？")
        }

        // 天气检查
        PreFlightCheckButton {
            name:       qsTr("天气")
            manualText: qsTr("风浪条件适合航行？无雷暴预警？")
        }

        // 通信检查
        PreFlightCheckButton {
            name:       qsTr("通信")
            manualText: qsTr("数传链路正常？视距范围内？")
        }

        // 载荷检查
        PreFlightCheckButton {
            name:       qsTr("载荷")
            telemetryFailure: {
                try { return activeVehicle ? activeVehicle.getFact("usvPayload.status").value === 3 : false }
                catch(e) { return false }
            }
            telemetryTextFailure: qsTr("载荷状态异常")
            manualText: qsTr("载荷已配置并启动？分光检测器供电正常？")
        }

        // 安全检查
        PreFlightCheckButton {
            name:       qsTr("安全")
            manualText: qsTr("周围无人员？已通知相关人员？")
        }
    }
}
}
