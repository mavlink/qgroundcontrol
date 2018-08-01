import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2
import QtQml                    2.2

import QGroundControl                   1.0
import QGroundControl.Airmap            1.0
import QGroundControl.Airspace          1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.SettingsManager   1.0

Item {
    id:                 _root
    implicitHeight:     detailCol.height
    implicitWidth:      detailCol.width
    property real baseHeight:  ScreenTools.defaultFontPixelHeight * 22
    property real baseWidth:   ScreenTools.defaultFontPixelWidth  * 40
    Column {
        id:             detailCol
        spacing:        ScreenTools.defaultFontPixelHeight * 0.25
        Rectangle {
            color:          qgcPal.windowShade
            anchors.right:  parent.right
            anchors.left:   parent.left
            height:         detailsLabel.height + ScreenTools.defaultFontPixelHeight
            QGCLabel {
                id:             detailsLabel
                text:           qsTr("Flight Details")
                font.pointSize: ScreenTools.mediumFontPointSize
                font.family:    ScreenTools.demiboldFontFamily
                anchors.centerIn: parent
            }
        }
        Item { width: 1; height: ScreenTools.defaultFontPixelHeight * 0.5; }
        Flickable {
            clip:           true
            width:          baseWidth
            height:         baseHeight
            contentHeight:  flContextCol.height
            flickableDirection: Flickable.VerticalFlick
            Column {
                id:                 flContextCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.right:      parent.right
                anchors.left:       parent.left
                QGCLabel {
                    text:           qsTr("Flight Date & Time")
                }
                Rectangle {
                    id:             dateRect
                    color:          qgcPal.windowShade
                    anchors.right:  parent.right
                    anchors.left:   parent.left
                    height:         datePickerCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    Column {
                        id:                 datePickerCol
                        spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                        anchors.margins:    ScreenTools.defaultFontPixelWidth
                        anchors.right:      parent.right
                        anchors.left:       parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        RowLayout {
                            spacing:        ScreenTools.defaultFontPixelWidth * 0.5
                            anchors.right:  parent.right
                            anchors.left:   parent.left
                            QGCButton {
                                text:       qsTr("Now")
                                onClicked: {
                                    _dirty = true
                                    var today = new Date()
                                    QGroundControl.airspaceManager.flightPlan.flightStartTime = today
                                    timeSlider.updateTime()
                                }
                            }
                            QGCButton {
                                text: {
                                    var today = QGroundControl.airspaceManager.flightPlan.flightStartTime
                                    if(datePicker.selectedDate.setHours(0,0,0,0) === today.setHours(0,0,0,0)) {
                                        return qsTr("Today")
                                    } else {
                                        return datePicker.selectedDate.toLocaleDateString(Qt.locale())
                                    }
                                }
                                Layout.fillWidth:   true
                                iconSource:         "qrc:/airmap/expand.svg"
                                onClicked: {
                                    _dirty = true
                                    datePicker.visible = true
                                }
                            }
                        }
                        Item {
                            anchors.right:  parent.right
                            anchors.left:   parent.left
                            height:         timeSlider.height
                            QGCLabel {
                                id:         timeLabel
                                text:       ('00' + hour).slice(-2) + ":" + ('00' + minute).slice(-2)
                                width:      ScreenTools.defaultFontPixelWidth * 5
                                anchors.left:  parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                property int hour:   Math.floor(timeSlider.value * 0.25)
                                property int minute: (timeSlider.value * 15) % 60
                            }
                            QGCSlider {
                                id:             timeSlider
                                width:          parent.width - timeLabel.width - ScreenTools.defaultFontPixelWidth
                                stepSize:       1
                                minimumValue:   0
                                maximumValue:   95 // 96 blocks of 15 minutes in 24 hours
                                anchors.right:  parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                onValueChanged: {
                                    _dirty = true
                                    var today = QGroundControl.airspaceManager.flightPlan.flightStartTime
                                    today.setHours(Math.floor(timeSlider.value * 0.25))
                                    today.setMinutes((timeSlider.value * 15) % 60)
                                    today.setSeconds(0)
                                    QGroundControl.airspaceManager.flightPlan.flightStartTime = today
                                }
                                Component.onCompleted: {
                                    updateTime()
                                }
                                function updateTime() {
                                    var today = QGroundControl.airspaceManager.flightPlan.flightStartTime
                                    var val = (((today.getHours() * 60) + today.getMinutes()) * (96/1440)) + 1
                                    if(val > 95) val = 95
                                    timeSlider.value = Math.ceil(val)
                                }
                            }
                        }
                    }
                }
                Item { width: 1; height: ScreenTools.defaultFontPixelHeight * 0.25; }
                QGCLabel {
                    text:           qsTr("Flight Context")
                    visible:        QGroundControl.airspaceManager.flightPlan.briefFeatures.count > 0
                }
                Repeater {
                    model:          QGroundControl.airspaceManager.flightPlan.briefFeatures
                    visible:        QGroundControl.airspaceManager.flightPlan.briefFeatures.count > 0
                    delegate:       FlightFeature {
                        feature:    object
                        visible:     object && object.type !== AirspaceRuleFeature.Unknown && object.description !== "" && object.name !== ""
                        anchors.right:  parent.right
                        anchors.left:   parent.left
                    }
                }
            }
        }
    }
    Calendar {
        id: datePicker
        anchors.centerIn:   parent
        visible:            false;
        minimumDate:        QGroundControl.airspaceManager.flightPlan.flightStartTime
        onClicked: {
            QGroundControl.airspaceManager.flightPlan.flightStartTime = datePicker.selectedDate
            visible = false;
        }
    }
}
