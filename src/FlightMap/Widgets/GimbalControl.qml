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
    id: rectangle
    height:     baseLayout.height + (_margins * 2)
    color:      "#80000000"
    radius:     _margins
    visible:    !QGroundControl.settingsManager.flyViewSettings.alternateInstrumentPanel.rawValue && hasGimbal() && multiVehiclePanelSelector.showSingleVehiclePanel

    property real   _margins:                                   ScreenTools.defaultFontPixelHeight / 2
    property var    _activeVehicle:                             QGroundControl.multiVehicleManager.activeVehicle

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
            id:                         titleLayout
            spacing:                    ScreenTools.defaultFontPixelHeight / 2
            height:                     parent.width
            Layout.alignment:           Qt.AlignHCenter


            RowLayout {
                Layout.alignment:       Qt.AlignHCenter

                Image {
                    id:                 navigatorImage
                    source:             "/qmlimages/gimbalNavigator.svg"
                    height:             ScreenTools.defaultFontPixelHeight
                    mipmap:             true
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    width:              height
                }

                QGCLabel {
                    Layout.alignment:   Qt.AlignLeft
                    text:               "Gimbal Orientation"
                    font.pointSize:     ScreenTools.defaultFontPointSize

                }

            }

            RowLayout {
                id:                         contentLayout
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
            }
        } // mainLayout


    } // ColumnLayout

    Component {
        id: gimbalSettingsDialogComponent

        QGCPopupDialog {
            title:      qsTr("Settings")
            buttons:    StandardButton.Close

            RowLayout {
                QGCLabel {
                    Layout.alignment: Qt.AlignLeft
                    text: "Change Gimbal mode (WIP):"
                    font.pointSize: ScreenTools.smallFontPointSize
                }

                QGCComboBox {
                    Layout.fillWidth:   true
                    sizeToContents:     true
                    model:              [ "---", "Retracted", "Neutral", "MAVLink targetting", "Rc Targetting", "GPS Point" ]
                    currentIndex:       0
                    visible:            true
                    // TODO This should send a MAVLINK_MSG_ID_MOUNT_CONTROL possibly taking arguments from settings or a ROI on screen
                    // onActivated:        _activeVehicle.gimbalMode(index)
                }
            }

        }
    }
}
