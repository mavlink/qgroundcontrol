/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0

//-------------------------------------------------------------------------
//-- GPS Indicator
Item {
    id:             _root
    width:          (gpsValuesColumn.x + gpsValuesColumn.width) * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator: true

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    QGCColoredImage {
        id:                 gpsIcon
        width:              height
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        source:             "/qmlimages/Gps.svg"
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
        opacity:            (_activeVehicle && _activeVehicle.gps.count.value >= 0) ? 1 : 0.5
        color:              qgcPal.buttonText
    }

    Column {
        id:                     gpsValuesColumn
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 2
        anchors.left:           gpsIcon.right

        QGCLabel {
            anchors.horizontalCenter:   hdopValue.horizontalCenter
            visible:                    _activeVehicle && !isNaN(_activeVehicle.gps.hdop.value)
            color:                      qgcPal.buttonText
            text:                       _activeVehicle ? _activeVehicle.gps.count.valueString : ""
        }

        QGCLabel {
            id:         hdopValue
            visible:    _activeVehicle && !isNaN(_activeVehicle.gps.hdop.value)
            color:      qgcPal.buttonText
            text:       _activeVehicle ? _activeVehicle.gps.hdop.value.toFixed(1) : ""
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked: {
            mainWindow.showIndicatorDrawer(gpsInfo)
        }
    }

    Component {
        id: gpsInfo

        ToolIndicatorPage {
            showExpand: true

            property real _margins: ScreenTools.defaultFontPixelHeight
            property real _editFieldWidth

            contentItem: Column {
                spacing:   ScreenTools.defaultFontPixelHeight * 0.5

                QGCLabel {
                    id:             gpsLabel
                    text:           (_activeVehicle && _activeVehicle.gps.count.value >= 0) ? qsTr("GPS Status") : qsTr("GPS Data Unavailable")
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                GridLayout {
                    id:                 gpsGrid
                    visible:            (_activeVehicle && _activeVehicle.gps.count.value >= 0)
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    columns: 2

                    QGCLabel { text: qsTr("GPS Count:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.count.valueString : qsTr("N/A", "No data to display") }
                    QGCLabel { text: qsTr("GPS Lock:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.lock.enumStringValue : qsTr("N/A", "No data to display") }
                    QGCLabel { text: qsTr("HDOP:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.hdop.valueString : qsTr("--.--", "No data to display") }
                    QGCLabel { text: qsTr("VDOP:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.vdop.valueString : qsTr("--.--", "No data to display") }
                    QGCLabel { text: qsTr("Course Over Ground:") }
                    QGCLabel { text: _activeVehicle ? _activeVehicle.gps.courseOverGround.valueString : qsTr("--.--", "No data to display") }
                }
            }
                
            expandedItem: IndicatorPageGroupLayout {
                heading: qsTr("RTK GPS Settings")

                QGCCheckBoxSlider {
                    Layout.fillWidth:   true
                    text:               qsTr("AutoConnect")
                    checked:            rtkAutocConnect.value
                    onClicked:          rtkAutocConnect.value = (checked ? true : false)

                    property Fact rtkAutocConnect: QGroundControl.settingsManager.autoConnectSettings.autoConnectRTKGPS
                }
            
                GridLayout {
                    id:         rtkGrid
                    columns:    3

                    property var  rtkSettings:      QGroundControl.settingsManager.rtkSettings
                    property bool useFixedPosition: rtkSettings.useFixedBasePosition.rawValue
                    property real firstColWidth:    ScreenTools.defaultFontPixelWidth * 5

                    QGCRadioButton {
                        text:               qsTr("Perform Survey-In")
                        visible:            rtkGrid.rtkSettings.useFixedBasePosition.visible
                        checked:            rtkGrid.rtkSettings.useFixedBasePosition.value === false
                        Layout.columnSpan:  3
                        onClicked:          rtkGrid.rtkSettings.useFixedBasePosition.value = false
                    }

                    Item { width: rtkGrid.firstColWidth; height: 1 }
                    QGCLabel {
                        text:       rtkGrid.rtkSettings.surveyInAccuracyLimit.shortDescription
                        visible:    rtkGrid.rtkSettings.surveyInAccuracyLimit.visible
                        enabled:    !rtkGrid.useFixedPosition
                    }
                    FactTextField {
                        Layout.preferredWidth:  editFieldWidth
                        fact:                   rtkGrid.rtkSettings.surveyInAccuracyLimit
                        visible:                rtkGrid.rtkSettings.surveyInAccuracyLimit.visible
                        enabled:                !rtkGrid.useFixedPosition
                    }

                    Item { width: rtkGrid.firstColWidth; height: 1 }
                    QGCLabel {
                        text:       rtkGrid.rtkSettings.surveyInMinObservationDuration.shortDescription
                        visible:    rtkGrid.rtkSettings.surveyInMinObservationDuration.visible
                        enabled:    !rtkGrid.useFixedPosition
                    }
                    FactTextField {
                        Layout.fillWidth:   true
                        fact:               rtkGrid.rtkSettings.surveyInMinObservationDuration
                        visible:            rtkGrid.rtkSettings.surveyInMinObservationDuration.visible
                        enabled:            !rtkGrid.useFixedPosition
                    }

                    QGCRadioButton {
                        text:               qsTr("Use Specified Base Position")
                        visible:            rtkGrid.rtkSettings.useFixedBasePosition.visible
                        checked:            rtkGrid.rtkSettings.useFixedBasePosition.value === true
                        onClicked:          rtkGrid.rtkSettings.useFixedBasePosition.value = true
                        Layout.columnSpan:  3
                    }

                    Item { width: rtkGrid.firstColWidth; height: 1 }
                    QGCLabel {
                        text:               rtkGrid.rtkSettings.fixedBasePositionLatitude.shortDescription
                        visible:            rtkGrid.rtkSettings.fixedBasePositionLatitude.visible
                        enabled:            rtkGrid.useFixedPosition
                    }
                    FactTextField {
                        Layout.fillWidth:   true
                        fact:               rtkGrid.rtkSettings.fixedBasePositionLatitude
                        visible:            rtkGrid.rtkSettings.fixedBasePositionLatitude.visible
                        enabled:            rtkGrid.useFixedPosition
                    }

                    Item { width: rtkGrid.firstColWidth; height: 1 }
                    QGCLabel {
                        text:               rtkGrid.rtkSettings.fixedBasePositionLongitude.shortDescription
                        visible:            rtkGrid.rtkSettings.fixedBasePositionLongitude.visible
                        enabled:            rtkGrid.useFixedPosition
                    }
                    FactTextField {
                        Layout.fillWidth:   true
                        fact:               rtkGrid.rtkSettings.fixedBasePositionLongitude
                        visible:            rtkGrid.rtkSettings.fixedBasePositionLongitude.visible
                        enabled:            rtkGrid.useFixedPosition
                    }

                    Item { width: rtkGrid.firstColWidth; height: 1 }
                    QGCLabel {
                        text:               rtkGrid.rtkSettings.fixedBasePositionAltitude.shortDescription
                        visible:            rtkGrid.rtkSettings.fixedBasePositionAltitude.visible
                        enabled:            rtkGrid.useFixedPosition
                    }
                    FactTextField {
                        Layout.fillWidth:   true
                        fact:               rtkGrid.rtkSettings.fixedBasePositionAltitude
                        visible:            rtkGrid.rtkSettings.fixedBasePositionAltitude.visible
                        enabled:            rtkGrid.useFixedPosition
                    }

                    Item { width: rtkGrid.firstColWidth; height: 1 }
                    QGCLabel {
                        text:               rtkGrid.rtkSettings.fixedBasePositionAccuracy.shortDescription
                        visible:            rtkGrid.rtkSettings.fixedBasePositionAccuracy.visible
                        enabled:            rtkGrid.useFixedPosition
                    }
                    FactTextField {
                        Layout.fillWidth:   true
                        fact:               rtkGrid.rtkSettings.fixedBasePositionAccuracy
                        visible:            rtkGrid.rtkSettings.fixedBasePositionAccuracy.visible
                        enabled:            rtkGrid.useFixedPosition
                    }

                    Item { width: rtkGrid.firstColWidth; height: 1 }
                    QGCButton {
                        text:               qsTr("Save Current Base Position")
                        enabled:            QGroundControl.gpsRtk && QGroundControl.gpsRtk.valid.value
                        Layout.columnSpan:  2
                        Layout.alignment:   Qt.AlignHCenter
                        onClicked: {
                            rtkGrid.rtkSettings.fixedBasePositionLatitude.rawValue =    QGroundControl.gpsRtk.currentLatitude.rawValue
                            rtkGrid.rtkSettings.fixedBasePositionLongitude.rawValue =   QGroundControl.gpsRtk.currentLongitude.rawValue
                            rtkGrid.rtkSettings.fixedBasePositionAltitude.rawValue =    QGroundControl.gpsRtk.currentAltitude.rawValue
                            rtkGrid.rtkSettings.fixedBasePositionAccuracy.rawValue =    QGroundControl.gpsRtk.currentAccuracy.rawValue
                        }
                    }
                }
            }
        }
    }
}
