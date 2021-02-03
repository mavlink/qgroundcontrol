/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.4
import QtPositioning            5.2
import QtQuick.Layouts          1.2
import QtQuick.Controls         1.4
import QtQuick.Dialogs          1.2
import QtGraphicalEffects       1.0

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controllers       1.0
import QGroundControl.FactSystem        1.0
import QGroundControl.FactControls      1.0

Rectangle {
    id:         _root
    height:     baseLayout.height + (_margins * 2)
    color:      "#80000000"
    radius:     _margins
    visible:    !QGroundControl.settingsManager.flyViewSettings.alternateInstrumentPanel.rawValue && hasGimbal() && multiVehiclePanelSelector.showSingleVehiclePanel

    property real   _margins:                                   ScreenTools.defaultFontPixelHeight / 2
    property var    _activeVehicle:                             QGroundControl.multiVehicleManager.activeVehicle
    property var    _defaultFocusAltitude:                      0.0

    function hasGimbal() {
        return _activeVehicle ? _activeVehicle.gimbalData : false
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    QGCColoredImage {
        anchors.margins:    _margins
        anchors.top:        parent.top
        anchors.right:      parent.right
        source:             "/res/gear-black.svg"
        mipmap:             true
        height:             ScreenTools.defaultFontPixelHeight
        width:              height
        sourceSize.height:  height
        color:              qgcPal.text
        fillMode:           Image.PreserveAspectFit
        visible:            true

        QGCMouseArea {
            fillItem:   parent
            onClicked:  mainWindow.showPopupDialogFromComponent(gimbalSettingsDialogComponent)
        }
    }

    ColumnLayout {
        id:                         baseLayout
        spacing:                    10
        anchors.margins:            _margins
        anchors.top:                parent.top
        anchors.horizontalCenter:   parent.horizontalCenter

        ColumnLayout {
            id:                         mainLayout
            spacing:                    ScreenTools.defaultFontPixelHeight / 2
            height:                     parent.width
            Layout.alignment:           Qt.AlignHCenter


            RowLayout {
                id:                     titleLayout
                Layout.alignment:       Qt.AlignHCenter

                Image {
                    id:                     navigatorImage
                    source:                 "/qmlimages/gimbalNavigator.svg"
                    height:                 ScreenTools.defaultFontPixelHeight
                    mipmap:                 true
                    fillMode:               Image.PreserveAspectFit
                    sourceSize.height:      height
                    width:                  height
                }

                QGCLabel {
                    Layout.alignment:       Qt.AlignLeft
                    text:                   "Gimbal Orientation"
                    font.pointSize:         ScreenTools.defaultFontPointSize
                }

            } // titleLayout

            RowLayout {
                id:                         angleLayout
                spacing:                    ScreenTools.defaultFontPixelHeight / 2
                height:                     parent.width
                Layout.alignment:           Qt.AlignHCenter

                ColumnLayout {
                    QGCLabel {
                        Layout.alignment:   Qt.AlignLeft
                        text:               "Tilt"
                        font.pointSize:     ScreenTools.smallFontPointSize

                    }

                    QGCLabel {
                        Layout.alignment:   Qt.AlignLeft
                        text:               "Roll"
                        font.pointSize:     ScreenTools.smallFontPointSize

                    }

                    QGCLabel {
                        Layout.alignment:   Qt.AlignLeft
                        text:               "Pan"
                        font.pointSize:     ScreenTools.smallFontPointSize

                    }
                }

                ColumnLayout {
                    QGCLabel {
                        Layout.alignment:   Qt.AlignRight
                        text:               _activeVehicle != null ? (_activeVehicle.gimbalPitch  / 100).toFixed(1) + "°" : "---°"
                        font.pointSize:     ScreenTools.smallFontPointSize

                    }

                    QGCLabel {
                        Layout.alignment:   Qt.AlignRight
                        text:               _activeVehicle != null ? (_activeVehicle.gimbalRoll / 100).toFixed(1) + "°" : "---°"
                        font.pointSize:     ScreenTools.smallFontPointSize

                    }

                    QGCLabel {
                        Layout.alignment:   Qt.AlignRight
                        text:               _activeVehicle != null ? (_activeVehicle.gimbalYaw / 100).toFixed(1) + "°" : "---°"
                        font.pointSize:     ScreenTools.smallFontPointSize

                    }
                }

            } // angleLayout

            RowLayout {
                id: gimbalTargetLayout

                QGCColoredImage {
                    id:                     _roiImage
                    source:                 "/qmlimages/roi.svg"
                    height:                 ScreenTools.defaultFontPixelHeight * 2
                    mipmap:                 true
                    fillMode:               Image.PreserveAspectFit
                    sourceSize.height:      height
                    width:                  height
                    color:                  qgcPal.text
                }

                QGCLabel {
                    id:                 _targetLabel
                    Layout.alignment:   Qt.AlignLeft
                    text:               "Click here to select a focus\ntarget for the gimbal."
                    font.pointSize:     ScreenTools.smallFontPointSize

                }

                QGCLabel {
                    id:                 _clickLabel
                    Layout.alignment:   Qt.AlignLeft
                    text:               "Click on the map to select a\ncoordinate or here to cancel."
                    font.pointSize:     ScreenTools.smallFontPointSize
                    visible:            false
                }

                QGCMouseArea {
                    fillItem:   parent
                    onClicked:  {
                        if (_clickLabel.visible === true) {
                            console.debug("GimbalControl: clearing callback")
                            _targetLabel.visible = true
                            _clickLabel.visible = false
                            _roiImage.color = qgcPal.text
                            _root.parent.mapControl.clearMapClickCallback("Gimbal Focus")
                            return
                        }

                        console.debug("GimbalControl: setting callback")
                        _targetLabel.visible = false
                        _clickLabel.visible = true
                        _roiImage.color = qgcPal.buttonHighlight

                        if (!_root.parent.mapControl.setMapClickCallback("Gimbal Focus", coord => {
                            if (_defaultFocusAltitude !== 0.0) {
                                coord.altitude = _defaultFocusAltitude
                            }

                            console.info("GimbalControl: Sending coordinates to gimbal", coord)
                            try {
                                _activeVehicle.sendCommand(1, 205 , true, coord.latitude * 1e7, coord.longitude * 1e7, coord.altitude * 100, 0, 0, 0, 4)
                            } catch (error) {
                                console.error("GimbalControl: Failed to set gimbal focus to ", coord, error)
                            }

                            _targetLabel.visible = true
                            _clickLabel.visible = false
                            _roiImage.color = qgcPal.text
                        })) {
                            console.log("GimbalControl: Failed to register callback")
                            _targetLabel.visible = true
                            _clickLabel.visible = false
                            _roiImage.color = qgcPal.text
                        }
                    }
                }

            }

        } // mainLayout


    } // ColumnLayout

    Component {
        id: gimbalSettingsDialogComponent

        QGCPopupDialog {
            title:      qsTr("Gimbal Settings")
            buttons:    StandardButton.Close

            GridLayout {
                columns: 2

                QGCLabel {
                    Layout.alignment: Qt.AlignLeft
                    text: "Change Gimbal mode:"
                    font.pointSize: ScreenTools.smallFontPointSize
                }

                QGCComboBox {
                    id:                 _modeSelector
                    Layout.fillWidth:   true
                    sizeToContents:     true
                    model:              [ "---", "Retracted", "Neutral", "MAVLink targetting", "Rc Targetting", "Track GPS Point", "Track System ID", "Track Home" ]
                    currentIndex:       0
                    visible:            true
                    onActivated:        _changeGimbalMode(index - 1)
                }

                QGCLabel {
                    Layout.alignment: Qt.AlignLeft
                    text: "Override coordinate altitude (m)"
                    font.pointSize: ScreenTools.smallFontPointSize
                }

                TextField {
                    id:               _focusAltitudeField
                    Layout.alignment: Qt.AlignLeft
                    validator:        DoubleValidator
                    inputMethodHints: Qt.ImhDigitsOnly
                    placeholderText: qsTr("Altitude (m)")
                    text:            _defaultFocusAltitude
                    onEditingFinished: {
                        _defaultFocusAltitude = parseFloat(_focusAltitudeField.text)

                    }
                }
            }
        }
    }

    function _changeGimbalMode(mode) {
        if (mode == 4 || mode == 5) {
            console.warn("GimbalControl: mode " + mode + " requires addition parameters and shouldn't be set without them")
            return false
        }
        if (mode < 0 || mode > 6) {
            console.warn("GimbalControl: unknown mode " + mode)
            return false
        }

        _activeVehicle.sendCommand(1, 205 , true, 0, 0, 0, 0, 0, 0, mode)
        return true
    }
}
