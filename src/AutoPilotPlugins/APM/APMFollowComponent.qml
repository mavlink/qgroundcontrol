/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

SetupPage {
    id:             followPage
    pageComponent:  followPageComponent

    FactPanelController {
        id:         controller
    }

    Component {
        id: followPageComponent

        ColumnLayout {
            id:      flowLayout
            spacing: _margins

            property Fact _followEnabled:               controller.getParameterFact(-1, "FOLL_ENABLE")
            property bool _followParamsAvailable:       controller.parameterExists(-1, "FOLL_SYSID")
            property Fact _followDistanceMax:           controller.getParameterFact(-1, "FOLL_DIST_MAX", false /* reportMissing */)
            property Fact _followSysId:                 controller.getParameterFact(-1, "FOLL_SYSID", false /* reportMissing */)
            property Fact _followOffsetX:               controller.getParameterFact(-1, "FOLL_OFS_X", false /* reportMissing */)
            property Fact _followOffsetY:               controller.getParameterFact(-1, "FOLL_OFS_Y", false /* reportMissing */)
            property Fact _followOffsetZ:               controller.getParameterFact(-1, "FOLL_OFS_Z", false /* reportMissing */)
            property Fact _followOffsetType:            controller.getParameterFact(-1, "FOLL_OFS_TYPE", false /* reportMissing */)
            property Fact _followAltitudeType:          controller.getParameterFact(-1, "FOLL_ALT_TYPE", false /* reportMissing */)
            property Fact _followYawBehavior:           controller.getParameterFact(-1, "FOLL_YAW_BEHAVE", false /* reportMissing */)
            property int  _followComboMaintainIndex:    0
            property int  _followComboSpecifyIndex:     1
            property bool _followMaintain:              followPositionCombo.currentIndex === _followComboMaintainIndex
            property bool _supportedSetup:              true
            property bool _roverFirmware:               controller.roverFirmware
            property bool _showMainSetup:               _followEnabled.rawValue == 1 && _supportedSetup
            property bool _showOffsetsSetup:            _showMainSetup && !_followMaintain

            readonly property int _followYawBehaviorNone:           0
            readonly property int _followYawBehaviorFace:           1
            readonly property int _followYawBehaviorSame:           2
            readonly property int _followYawBehaviorFlight:         3
            readonly property int _followAltitudeTypeAbsolute:      0
            readonly property int _followAltitudeTypeRelative:      1
            readonly property int _followOffsetTypeRelative:        1

            Component.onCompleted: _setUIFromParams()

            function validateSupportedParamSetup() {
                var followSysIdOk = _followSysId.rawValue == QGroundControl.mavlinkSystemID
                var followOffsetOk = _followOffsetType.rawValue == _followOffsetTypeRelative
                var followAltOk = true
                var followYawOk = true
                if (!_roverFirmware) {
                    followAltOk = _followAltitudeType.rawValue == _followAltitudeTypeRelative
                    followYawOk = _followYawBehavior.rawValue == _followYawBehaviorNone || _followYawBehavior.rawValue == _followYawBehaviorFace || _followYawBehavior.rawValue == _followYawBehaviorFlight
                }
                _supportedSetup = followOffsetOk && followAltOk && followYawOk && followSysIdOk
                console.log("_supportedSetup", _supportedSetup, followSysIdOk, followOffsetOk, followAltOk, followYawOk)
                return _supportedSetup
            }

            function _setUIFromParams() {
                if (!_followParamsAvailable || !validateSupportedParamSetup()) {
                    return
                }

                if (_followOffsetX.rawValue == 0 && _followOffsetY.rawValue == 0 && _followOffsetZ.rawValue == 0) {
                    followPositionCombo.currentIndex =_followComboMaintainIndex
                    controller.distance.rawValue = 0
                    controller.angle.rawValue = 0
                    controller.height.rawValue = 0
                } else {
                    followPositionCombo.currentIndex =_followComboSpecifyIndex
                    var angleRadians = Math.atan2(_followOffsetX.rawValue, _followOffsetY.rawValue)
                    if (angleRadians == 0) {
                        controller.distance.rawValue = _followOffsetY.rawValue
                    } else {
                        controller.distance.rawValue = _followOffsetX.rawValue / Math.sin(angleRadians)
                    }
                    controller.angle.rawValue = _radiansToHeading(angleRadians)
                }
                controller.height.rawValue = -_followOffsetZ.rawValue
                if (!_roverFirmware) {
                    var comboIndex = -1
                    for (var i=0; i<pointVehicleCombo.rgValues.length; i++) {
                        if (pointVehicleCombo.rgValues[i] == _followYawBehavior.rawValue) {
                            comboIndex = i
                            break
                        }
                    }

                    pointVehicleCombo.currentIndex = comboIndex
                }
            }

            function _setFollowMeParamDefaults() {
                _followSysId.rawValue = QGroundControl.mavlinkSystemID
                _followOffsetType.rawValue = _followOffsetTypeRelative
                if (!_roverFirmware) {
                    _followAltitudeType.rawValue = _followAltitudeTypeRelative
                    _followYawBehavior.rawValue = _followYawBehaviorFace
                }

                controller.distance.value = controller.distance.defaultValue
                controller.angle.value = controller.angle.defaultValue
                controller.height.value = controller.height.defaultValue

                _setXYOffsetByAngleAndDistance(controller.angle.rawValue, controller.distance.rawValue)
                _followOffsetZ.rawValue = -controller.height.rawValue
                _setUIFromParams()
            }

            function _setXYOffsetByAngleAndDistance(headingAngleDegrees, distance) {
                if (distance == 0) {
                    _followOffsetX.rawValue = _followOffsetY.rawValue = 0
                } else {
                    var angleRadians = _headingToRadians(headingAngleDegrees)
                    if (angleRadians == 0) {
                        _followOffsetX.rawValue = 0
                        _followOffsetY.rawValue = distance
                    } else {
                        _followOffsetX.rawValue = Math.sin(angleRadians) * distance
                        _followOffsetY.rawValue = Math.cos(angleRadians) * distance
                        if (Math.abs(_followOffsetX.rawValue) < 0.0001) {
                            _followOffsetX.rawValue = 0
                        }
                        if (Math.abs(_followOffsetY.rawValue) < 0.0001) {
                            _followOffsetY.rawValue = 0
                        }
                    }
                }
            }

            function _radiansToHeading(radians) {
                var geometricAngle = QGroundControl.unitsConversion.radiansToDegrees(radians)
                var headingAngle = 90 - geometricAngle
                if (headingAngle < 0) {
                    headingAngle += 360
                } else if (headingAngle > 360) {
                    headingAngle -= 360
                }
                return headingAngle
            }

            function _headingToRadians(heading) {
                var geometricAngle = -(heading - 90)
                return QGroundControl.unitsConversion.degreesToRadians(geometricAngle)
            }

            APMFollowComponentController {
                id: controller

                onMissingParametersAvailable: {
                    _followDistanceMax =    controller.getParameterFact(-1, "FOLL_DIST_MAX")
                    _followSysId =          controller.getParameterFact(-1, "FOLL_SYSID")
                    _followOffsetX =        controller.getParameterFact(-1, "FOLL_OFS_X")
                    _followOffsetY =        controller.getParameterFact(-1, "FOLL_OFS_Y")
                    _followOffsetZ =        controller.getParameterFact(-1, "FOLL_OFS_Z")
                    _followOffsetType =     controller.getParameterFact(-1, "FOLL_OFS_TYPE")
                    if (!_roverFirmware) {
                        _followAltitudeType =   controller.getParameterFact(-1, "FOLL_ALT_TYPE")
                        _followYawBehavior =    controller.getParameterFact(-1, "FOLL_YAW_BEHAVE")
                    }

                    _followParamsAvailable = true
                    vehicleParamRefreshLabel.visible = false
                    validateSupportedParamSetup()
                }
            }

            QGCPalette { id: ggcPal; colorGroupEnabled: true }

            QGCCheckBox {
                text:       qsTr("Enable Follow Me")
                checked:    _followEnabled.rawValue == 1
                onClicked: {
                    if (checked) {
                        _followEnabled.rawValue = 1
                        var missingParameters = [ "FOLL_DIST_MAX", "FOLL_SYSID", "FOLL_OFS_X", "FOLL_OFS_Y", "FOLL_OFS_Z", "FOLL_OFS_TYPE"  ]
                        if (!_roverFirmware) {
                            missingParameters.push("FOLL_ALT_TYPE", "FOLL_YAW_BEHAVE")
                        }
                        controller.getMissingParameters(missingParameters)
                        vehicleParamRefreshLabel.visible = true
                    } else {
                        _followEnabled.rawValue = 0
                    }
                }
            }

            QGCLabel {
                id:         vehicleParamRefreshLabel
                text:       qsTr("Waiting for Vehicle to update")
                visible:    false
            }

            Column {
                width:      offsetSetupLayout.width
                spacing:    ScreenTools.defaultFontPixelWidth
                visible:    !_supportedSetup

                QGCLabel {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("The vehicle parameters required for follow me are currently set in a way which is not supported. Using follow with this setup may lead to unpredictable/hazardous results.")
                    wrapMode:       Text.WordWrap
                    onWidthChanged: console.log('width', width)
                }

                QGCButton {
                    text:       qsTr("Reset To Supported Settings")
                    onClicked:  _setFollowMeParamDefaults()
                }
            }

            ColumnLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth
                visible:            _showMainSetup

                ColumnLayout {
                    Layout.fillWidth:   true
                    spacing:            ScreenTools.defaultFontPixelWidth

                    GridLayout {
                        Layout.fillWidth:   true
                        columns:            2

                        QGCLabel { text: qsTr("Vehicle Position") }
                        QGCComboBox {
                            id:                 followPositionCombo
                            Layout.fillWidth:   true
                            model:              [ qsTr("Maintain Current Offsets"), qsTr("Specify Offsets")]

                            onActivated: {
                                if (index == 0) {
                                    _followOffsetX.rawValue = _followOffsetY.rawValue = _followOffsetZ.rawValue = 0
                                    _setUIFromParams()
                                } else {
                                    _setFollowMeParamDefaults()
                                }
                            }
                        }

                        QGCLabel {
                            text:       qsTr("Point Vehicle")
                            visible:    !_roverFirmware
                        }
                        QGCComboBox {
                            id:                     pointVehicleCombo
                            Layout.fillWidth:       true
                            model:                  rgText
                            visible:                !_roverFirmware
                            onActivated:            _followYawBehavior.rawValue = rgValues[index]

                            property var rgText:    [ qsTr("Maintain current vehicle orientation"), qsTr("Point at ground station location"), qsTr("Same direction as ground station movement") ]
                            property var rgValues:  [ _followYawBehaviorNone, _followYawBehaviorFace, _followYawBehaviorFlight ]
                        }
                    }

                    GridLayout {
                        Layout.fillWidth:   true
                        columns:            4
                        visible:            !_followMaintain

                        QGCLabel {
                            Layout.columnSpan:  2
                            Layout.alignment:   Qt.AlignHCenter
                            text:               qsTr("Vehicle Offsets")
                        }

                        QGCLabel { text: qsTr("Angle") }
                        FactTextField {
                            fact:       controller.angle
                            onUpdated:  { console.log("updated"); _setXYOffsetByAngleAndDistance(controller.angle.rawValue, controller.distance.rawValue) }
                        }

                        QGCLabel { text: qsTr("Distance") }
                        FactTextField {
                            fact:       controller.distance
                            onUpdated:  _setXYOffsetByAngleAndDistance(controller.angle.rawValue, controller.distance.rawValue)
                        }

                        QGCLabel {
                            id:         heightLabel
                            text:       qsTr("Height")
                            visible:    !_roverFirmware && !_followMaintain
                        }
                        FactTextField {
                            fact:       controller.height
                            visible:    heightLabel.visible
                            onUpdated:  _followOffsetZ.rawValue = -controller.height.rawValue
                        }
                    }
                }
            }

            RowLayout {
                id:         offsetSetupLayout
                spacing:    ScreenTools.defaultFontPixelWidth * 2
                visible:    _showOffsetsSetup

                Item {
                    height: ScreenTools.defaultFontPixelWidth * 50
                    width:  height

                    Rectangle {
                        anchors.top:                parent.top
                        anchors.bottom:             parent.bottom
                        anchors.horizontalCenter:   parent.horizontalCenter
                        width:                      3
                        color:                      qgcPal.windowShade
                    }

                    Rectangle {
                        anchors.left:           parent.left
                        anchors.right:          parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        height:                 3
                        color:                  qgcPal.windowShade
                    }

                    QGCLabel {
                        anchors.horizontalCenter:   parent.horizontalCenter
                        anchors.topMargin:          parent.height / 4
                        anchors.top:                parent.top
                        text:                       qsTr("Click in the graphic to change angle")
                        opacity:                    0.5
                    }

                    Image {
                        id:                 gcsIcon
                        anchors.centerIn:   parent
                        source:             "/res/QGCLogoArrow"
                        mipmap:             true
                        antialiasing:       true
                        fillMode:           Image.PreserveAspectFit
                        height:             ScreenTools.defaultFontPixelHeight * 2.5
                        sourceSize.height:  height
                    }

                    Item {
                        id:             vehicleHolder
                        anchors.fill:   parent

                        transform: Rotation {
                            origin.x:       vehicleHolder.width  / 2
                            origin.y:       vehicleHolder.height / 2
                            angle:          controller.angle.rawValue
                        }

                        Image {
                            id:                 vehicleIcon
                            anchors.top:        parent.top
                            anchors.horizontalCenter: parent.horizontalCenter
                            source:             controller.vehicle.vehicleImageOpaque
                            mipmap:             true
                            height:             ScreenTools.defaultFontPixelHeight * 2.5
                            sourceSize.height:  height
                            fillMode:           Image.PreserveAspectFit

                            transform: Rotation {
                                origin.x:       vehicleIcon.width  / 2
                                origin.y:       vehicleIcon.height / 2
                                angle:          _roverFirmware ? 0 :
                                                                 (_followYawBehavior.rawValue == _followYawBehaviorNone ?
                                                                      0 :
                                                                      (_followYawBehavior.rawValue == _followYawBehaviorFace ?
                                                                           180 :
                                                                           -controller.angle.rawValue))
                            }
                        }

                        Rectangle {
                            id:         distanceLine
                            x:          parent.width / 2
                            y:          vehicleIcon.height
                            height:     (parent.height / 2) - (vehicleIcon.height + (gcsIcon.height / 2))
                            width:      2
                            color:      qgcPal.text
                            opacity:    0.4

                            Rectangle {
                                anchors.top:                parent.top
                                anchors.horizontalCenter:   parent.horizontalCenter
                                width:                      ScreenTools.defaultFontPixelWidth * 2
                                height:                     2
                                color:                      qgcPal.text
                            }

                            Rectangle {
                                anchors.bottom:             parent.bottom
                                anchors.horizontalCenter:   parent.horizontalCenter
                                width:                      ScreenTools.defaultFontPixelWidth * 2
                                height:                     2
                                color:                      qgcPal.text
                            }
                        }

                        QGCLabel {
                            id:                 distanceLabel
                            anchors.centerIn:   distanceLine
                            text:               controller.distance.valueString + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString

                            transform: Rotation {
                                origin.x:       distanceLabel.width  / 2
                                origin.y:       distanceLabel.height / 2
                                angle:          -controller.angle.rawValue
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent

                        onClicked: {
                            // Translate x,y to centered
                            var x = mouse.x - (width / 2)
                            var y = (height - mouse.y) - (height / 2)
                            controller.angle.rawValue = _radiansToHeading(Math.atan2(y, x))
                            _setXYOffsetByAngleAndDistance(controller.angle.rawValue, controller.distance.rawValue)
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillHeight:  true
                    spacing:            0
                    visible:            !_roverFirmware

                    Image {
                        id:                 vehicleIconHeight
                        source:             controller.vehicle.vehicleImageOpaque
                        mipmap:             true
                        height:             ScreenTools.defaultFontPixelHeight * 2.5
                        sourceSize.height:  height
                        fillMode:           Image.PreserveAspectFit

                        transform: Rotation {
                            origin.x:       vehicleIconHeight.width  / 2
                            origin.y:       vehicleIconHeight.height / 2
                            angle:          65
                            axis { x: 1; y: 0; z: 0 }
                        }
                    }

                    Item {
                        Layout.alignment:   Qt.AlignHCenter
                        Layout.fillHeight:  true
                        width:              Math.max(ScreenTools.defaultFontPixelWidth * 2, heightValueLabel.width)

                        Rectangle {
                            id:                         heightLine
                            anchors.top:                parent.top
                            anchors.bottom:             parent.bottom
                            anchors.horizontalCenter:   parent.horizontalCenter
                            width:                      2
                            color:                      qgcPal.text
                            opacity:                    0.4

                            Rectangle {
                                anchors.top:                parent.top
                                anchors.horizontalCenter:   parent.horizontalCenter
                                width:                      ScreenTools.defaultFontPixelWidth * 2
                                height:                     2
                                color:                      qgcPal.text
                            }

                            Rectangle {
                                anchors.bottom:             parent.bottom
                                anchors.horizontalCenter:   parent.horizontalCenter
                                width:                      ScreenTools.defaultFontPixelWidth * 2
                                height:                     2
                                color:                      qgcPal.text
                            }
                        }

                        QGCLabel {
                            id:                 heightValueLabel
                            anchors.centerIn:   parent
                            text:               controller.height.valueString + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
                        }
                    }

                    MissionItemIndexLabel {
                        id:                 launchIconHeight
                        Layout.alignment:   Qt.AlignHCenter
                        label:              qsTr("L")

                        transform: [
                            Scale {
                                origin.x:       launchIconHeight.width  / 2
                                origin.y:       launchIconHeight.height / 2
                                xScale:         1.5
                                yScale:         2.5

                            },
                            Rotation {
                                origin.x:       launchIconHeight.width  / 2
                                origin.y:       launchIconHeight.height / 2
                                angle:          75
                                axis { x: 1; y: 0; z: 0 }
                            } ]
                    }
                }
            }
        }
    }
}
