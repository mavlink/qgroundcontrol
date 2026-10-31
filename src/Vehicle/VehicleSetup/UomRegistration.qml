import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id: root
    objectName: "uomRegistrationPage"
    color: QGroundControl.globalPalette.window

    property var vehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var manager: vehicle ? vehicle.uomRegistrationManager : null

    QGCFlickable {
        objectName: "uomRegistrationFlickable"
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelWidth
        contentWidth: width
        contentHeight: content.implicitHeight
        clip: true

        ColumnLayout {
            id: content
            width: parent.width
            spacing: ScreenTools.defaultFontPixelHeight

            QGCLabel {
                Layout.fillWidth: true
                text: root.manager ? root.manager.deviceStatus : qsTr("Connect an aircraft to manage UOM registration")
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
            }

            RowLayout {
                visible: root.manager && root.manager.available

                QGCButton {
                    objectName: "uomReadDeviceButton"
                    text: qsTr("Read activation devices")
                    enabled: root.manager && !root.manager.busy
                    onClicked: root.manager.refreshDevice()
                }

                QGCComboBox {
                    objectName: "uomDeviceSelector"
                    visible: root.manager && root.manager.devices.length > 1
                    model: root.manager ? root.manager.devices : []
                    currentIndex: root.manager ? root.manager.selectedDevice : -1
                    enabled: root.manager && !root.manager.busy && !root.manager.pending
                    onActivated: (index) => root.manager.selectDevice(index)
                }
            }

            UomRegistrationPanel {
                Layout.fillWidth: true
                manager: root.manager
            }
        }
    }
}
