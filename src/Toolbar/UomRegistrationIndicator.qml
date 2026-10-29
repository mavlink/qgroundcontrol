pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id: root
    objectName: "toolbar_uomRegistrationIndicator"
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    width: title.implicitWidth + ScreenTools.defaultFontPixelWidth

    readonly property var vehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var manager: vehicle ? vehicle.uomRegistrationManager : null
    property bool showIndicator: manager && manager.available

    QGCLabel {
        id: title
        anchors.centerIn: parent
        text: qsTr("UOM")
    }

    MouseArea {
        anchors.fill: parent
        onClicked: mainWindow.showIndicatorDrawer(statusPage, root)
    }

    Component {
        id: statusPage

        ToolIndicatorPage {
            showExpand: false

            contentComponent: ColumnLayout {
                width: ScreenTools.defaultFontPixelWidth * 42
                spacing: ScreenTools.defaultFontPixelHeight / 2

                QGCLabel {
                    Layout.fillWidth: true
                    font.bold: true
                    text: qsTr("UOM registration and activation")
                    wrapMode: Text.WordWrap
                }

                QGCLabel {
                    Layout.fillWidth: true
                    objectName: "toolbar_uomRegistrationState"
                    text: qsTr("Registration: %1").arg(root.manager ? root.manager.registrationText : qsTr("Unknown"))
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                }

                QGCLabel {
                    Layout.fillWidth: true
                    objectName: "toolbar_uomDeviceState"
                    text: qsTr("Device: %1").arg(root.manager ? root.manager.deviceActivationText : qsTr("Unknown"))
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                }

                QGCLabel {
                    Layout.fillWidth: true
                    objectName: "toolbar_uomReportState"
                    text: qsTr("UOM acknowledgement: %1").arg(root.manager ? root.manager.reportText : qsTr("Unknown"))
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                }

                QGCButton {
                    objectName: "toolbar_uomRegistrationButton"
                    text: qsTr("Manage registration")
                    onClicked: {
                        mainWindow.closeIndicatorDrawer()
                        mainWindow.showUomRegistrationPage()
                    }
                }
            }
        }
    }
}
