/****************************************************************************
 *
 * USV Integrated Instrument Panel - 无人船综合仪表盘
 *
 * 整合罗盘、航行状态和姿态监测
 * 替换默认的 IntegratedCompassAttitude.qml
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

Item {
    id:             control
    implicitWidth:  mainLayout.width + (_toolsMargin * 2)
    implicitHeight: mainLayout.height + (_toolsMargin * 2)

    // ========== 与原版 IntegratedCompassAttitude 兼容的接口属性 ==========
    property real attitudeSize:                 ScreenTools.defaultFontPixelWidth * 2
    property real attitudeSpacing:              attitudeSize / 2
    property real extraInset:                   attitudeSize + attitudeSpacing
    property real extraValuesWidth:             _compassSize / 2
    property real defaultCompassRadius:         (mainWindow.width * 0.15) / 2
    property real maxCompassRadius:             ScreenTools.defaultFontPixelHeight * 7 / 2
    property real compassRadius:                Math.min(defaultCompassRadius, maxCompassRadius)
    property real compassBorder:                ScreenTools.defaultFontPixelHeight / 2
    property var  vehicle:                      globals.activeVehicle
    property bool usedByMultipleVehicleList:    false

    // ========== USV 自定义属性 ==========
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75
    property real   _compassSize:           Math.min(ScreenTools.defaultFontPixelHeight * 7, mainWindow.width * 0.12)

    // 姿态数据 - 添加空值检查
    property real   roll:                   vehicle && vehicle.roll ? vehicle.roll.rawValue : 0
    property real   pitch:                  vehicle && vehicle.pitch ? vehicle.pitch.rawValue : 0

    // 姿态警告阈值
    property real   rollWarningThreshold:   15.0
    property real   pitchWarningThreshold:  10.0
    property real   rollCriticalThreshold:  25.0
    property real   pitchCriticalThreshold: 20.0

    property bool   isRollWarning:          Math.abs(roll) > rollWarningThreshold
    property bool   isPitchWarning:         Math.abs(pitch) > pitchWarningThreshold
    property bool   isRollCritical:         Math.abs(roll) > rollCriticalThreshold
    property bool   isPitchCritical:        Math.abs(pitch) > pitchCriticalThreshold
    property bool   isAttitudeWarning:      isRollWarning || isPitchWarning
    property bool   isAttitudeCritical:     isRollCritical || isPitchCritical

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Rectangle {
        anchors.fill:   mainLayout
        anchors.margins: -_toolsMargin
        color:          qgcPal.window
        radius:         ScreenTools.defaultFontPixelWidth / 2
        opacity:        0.85
    }

    RowLayout {
        id:         mainLayout
        spacing:    _toolsMargin * 2

        // ========== 左侧：数据面板 ==========
        ColumnLayout {
            id:                 dataPanel
            Layout.alignment:   Qt.AlignVCenter
            spacing:            _toolsMargin

            // -------- 航行状态 --------
            ColumnLayout {
                spacing: 2

                QGCLabel {
                    text:       qsTr("航行状态")
                    font.bold:  true
                    font.pointSize: ScreenTools.smallFontPointSize
                }

                GridLayout {
                    columns:        4
                    rowSpacing:     2
                    columnSpacing:  _toolsMargin

                    QGCLabel { text: qsTr("航速:"); opacity: 0.7; font.pointSize: ScreenTools.smallFontPointSize }
                    QGCLabel {
                        text: vehicle && vehicle.groundSpeed ? vehicle.groundSpeed.rawValue.toFixed(1) + " m/s" : "---"
                        font.bold: true
                        font.pointSize: ScreenTools.smallFontPointSize
                    }
                    QGCLabel { text: qsTr("航向:"); opacity: 0.7; font.pointSize: ScreenTools.smallFontPointSize }
                    QGCLabel {
                        text: vehicle && vehicle.heading ? vehicle.heading.rawValue.toFixed(0) + "°" : "---"
                        font.bold: true
                        font.pointSize: ScreenTools.smallFontPointSize
                    }

                    QGCLabel { text: qsTr("油门:"); opacity: 0.7; font.pointSize: ScreenTools.smallFontPointSize }
                    QGCLabel {
                        text: vehicle && vehicle.throttlePct !== undefined ? (vehicle.throttlePct * 100).toFixed(0) + "%" : "---"
                        font.bold: true
                        font.pointSize: ScreenTools.smallFontPointSize
                    }
                    QGCLabel { text: qsTr("距Home:"); opacity: 0.7; font.pointSize: ScreenTools.smallFontPointSize }
                    QGCLabel {
                        text: vehicle && vehicle.distanceToHome ? vehicle.distanceToHome.rawValue.toFixed(0) + "m" : "---"
                        font.bold: true
                        font.pointSize: ScreenTools.smallFontPointSize
                    }
                }
            }

            // 分隔线
            Rectangle {
                Layout.fillWidth:   true
                height:             1
                color:              qgcPal.text
                opacity:            0.2
            }

            // -------- 姿态监测 --------
            Rectangle {
                Layout.fillWidth:   true
                implicitHeight:     attitudeContent.height + _toolsMargin
                color:              isAttitudeCritical ? qgcPal.colorRed :
                                    (isAttitudeWarning ? qgcPal.colorOrange : "transparent")
                radius:             ScreenTools.defaultFontPixelWidth / 4

                ColumnLayout {
                    id:             attitudeContent
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: _toolsMargin / 2
                    spacing:        2

                    // 标题行：姿态监测 + 状态
                    RowLayout {
                        Layout.fillWidth: true

                        QGCLabel {
                            text:       qsTr("姿态监测")
                            font.bold:  true
                            font.pointSize: ScreenTools.smallFontPointSize
                            color:      isAttitudeCritical ? "white" : qgcPal.text
                        }

                        Item { Layout.fillWidth: true }

                        QGCLabel {
                            text: isAttitudeCritical ? qsTr("⚠ 危险") :
                                  (isAttitudeWarning ? qsTr("⚠ 异常") : qsTr("✓ 平稳"))
                            font.bold:  true
                            font.pointSize: ScreenTools.smallFontPointSize
                            color: isAttitudeCritical ? "white" :
                                   (isAttitudeWarning ? qgcPal.colorOrange : qgcPal.colorGreen)
                        }
                    }

                    // 数据行：横滚 | 俯仰
                    RowLayout {
                        spacing: _toolsMargin * 2

                        RowLayout {
                            spacing: _toolsMargin / 2
                            QGCLabel {
                                text: qsTr("横滚:");
                                opacity: isAttitudeCritical ? 1.0 : 0.7
                                font.pointSize: ScreenTools.smallFontPointSize
                                color: isAttitudeCritical ? "white" : qgcPal.text
                            }
                            QGCLabel {
                                text: roll.toFixed(1) + "°"
                                font.bold: true
                                font.pointSize: ScreenTools.smallFontPointSize
                                color: isRollCritical ? "white" :
                                       (isRollWarning ? qgcPal.colorOrange : qgcPal.text)
                            }
                        }

                        RowLayout {
                            spacing: _toolsMargin / 2
                            QGCLabel {
                                text: qsTr("俯仰:");
                                opacity: isAttitudeCritical ? 1.0 : 0.7
                                font.pointSize: ScreenTools.smallFontPointSize
                                color: isAttitudeCritical ? "white" : qgcPal.text
                            }
                            QGCLabel {
                                text: pitch.toFixed(1) + "°"
                                font.bold: true
                                font.pointSize: ScreenTools.smallFontPointSize
                                color: isPitchCritical ? "white" :
                                       (isPitchWarning ? qgcPal.colorOrange : qgcPal.text)
                            }
                        }
                    }
                }
            }

        }

        // ========== 右侧：罗盘 ==========
        QGCCompassWidget {
            id:                 compassWidget
            Layout.alignment:   Qt.AlignVCenter
            size:               _compassSize
            vehicle:            control.vehicle
        }
    }
}
