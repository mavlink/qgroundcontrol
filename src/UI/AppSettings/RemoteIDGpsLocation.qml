import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    Layout.fillWidth: true
    implicitHeight:   _column.implicitHeight

    property real _comboFieldWidth: ScreenTools.defaultFontPixelWidth * 30
    property real _valueFieldWidth: ScreenTools.defaultFontPixelWidth * 10
    property int  _locationType:   QGroundControl.settingsManager.remoteIDSettings.locationType.value

    property var    gcsPosition: QGroundControl.qgcPositionManger.gcsPosition
    property real   gcsHDOP:     QGroundControl.qgcPositionManger.gcsPositionHorizontalAccuracy
    property string gpsDisabled: "Disabled"
    property string gpsUdpPort:  "UDP Port"

    ColumnLayout {
        id:     _column
        anchors.left:   parent.left
        anchors.right:  parent.right
        spacing:        ScreenTools.defaultFontPixelHeight * 0.5

        GridLayout {
            visible:            !ScreenTools.isMobile
                                && QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.visible
                                && QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.visible
                                && _locationType !== RemoteIDSettings.LocationType.TAKEOFF
            Layout.fillWidth:   true
            rowSpacing:         ScreenTools.defaultFontPixelWidth * 3
            columns:            2
            columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
            Layout.alignment:   Qt.AlignHCenter

            QGCLabel {
                text: qsTr("NMEA External GPS Device")
            }
            QGCComboBox {
                id:                     nmeaPortCombo
                Layout.preferredWidth:  _comboFieldWidth

                model: ListModel { }

                onActivated: (index) => {
                    if (index !== -1) {
                        QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.value = textAt(index);
                    }
                }
                Component.onCompleted: {
                    model.append({text: gpsDisabled})
                    model.append({text: gpsUdpPort})

                    for (var i in QGroundControl.linkManager.serialPorts) {
                        nmeaPortCombo.model.append({text: QGroundControl.linkManager.serialPorts[i]})
                    }
                    var index = nmeaPortCombo.find(QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.valueString);
                    nmeaPortCombo.currentIndex = index;
                    if (QGroundControl.linkManager.serialPorts.length === 0) {
                        nmeaPortCombo.model.append({text: "Serial <none available>"})
                    }
                }
            }

            QGCLabel {
                visible: nmeaPortCombo.currentText !== gpsUdpPort && nmeaPortCombo.currentText !== gpsDisabled
                text:    qsTr("NMEA GPS Baudrate")
            }
            QGCComboBox {
                visible:                nmeaPortCombo.currentText !== gpsUdpPort && nmeaPortCombo.currentText !== gpsDisabled
                id:                     nmeaBaudCombo
                Layout.preferredWidth:  _comboFieldWidth
                model:                  [1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600]

                onActivated: (index) => {
                    if (index !== -1) {
                        QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.value = textAt(index);
                    }
                }
                Component.onCompleted: {
                    var index = nmeaBaudCombo.find(QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.valueString);
                    nmeaBaudCombo.currentIndex = index;
                }
            }

            QGCLabel {
                text:       qsTr("NMEA stream UDP port")
                visible:    nmeaPortCombo.currentText === gpsUdpPort
            }
            FactTextField {
                visible:                nmeaPortCombo.currentText === gpsUdpPort
                Layout.preferredWidth:  _valueFieldWidth
                fact:                   QGroundControl.settingsManager.autoConnectSettings.nmeaUdpPort
            }
        }

        GridLayout {
            Layout.fillWidth:   true
            columns:            2
            columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
            visible:            gcsPosition.isValid

            QGCLabel { text: qsTr("Latitude:")  }
            QGCLabel { text: gcsPosition.isValid ? gcsPosition.latitude.toFixed(7)  : qsTr("N/A") }

            QGCLabel { text: qsTr("Longitude:") }
            QGCLabel { text: gcsPosition.isValid ? gcsPosition.longitude.toFixed(7) : qsTr("N/A") }

            QGCLabel { text: qsTr("HDOP:")      }
            QGCLabel { text: gcsHDOP > 0 ? gcsHDOP.toFixed(1) + " m" : qsTr("N/A") }
        }
    }
}
