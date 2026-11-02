import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id:                     control
    Layout.preferredWidth:  mainLayout.width

    property bool showIndicator:     true
    property bool waitForParameters: false
    property real fontPointSize:     ScreenTools.largeFontPointSize
    property var activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle

    readonly property bool _isVTOL:          activeVehicle ? activeVehicle.vtol : false
    readonly property bool _vtolInFWDFlight: activeVehicle ? activeVehicle.vtolInFwdFlight : false
    readonly property bool _vehicleInAir:    activeVehicle ? activeVehicle.airborne || activeVehicle.landing : false

    QGCPalette { id: qgcPal }

    RowLayout {
        id:                     mainLayout
        anchors.verticalCenter: parent.verticalCenter
        spacing:                ScreenTools.defaultFontPixelWidth / 2

        QGCColoredImage {
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 3
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight
            fillMode:               Image.PreserveAspectFit
            mipmap:                 true
            color:                  qgcPal.text
            source:                 "/qmlimages/FlightModesComponentIcon.png"
        }

        QGCLabel {
            text:           control.activeVehicle ? control.activeVehicle.flightMode : qsTr("N/A", "No data to display")
            color:          qgcPal.text
            font.pointSize: control.fontPointSize
        }

        QGCLabel {
            Layout.alignment:    Qt.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text:                control._vtolInFWDFlight ? qsTr("FW\nVTOL") : qsTr("MR\nVTOL")
            font.pointSize:      ScreenTools.smallFontPointSize
            wrapMode:            Text.WordWrap
            visible:             control._isVTOL
        }
    }

    MouseArea {
        anchors.fill: mainLayout
        enabled:      !!control.activeVehicle
        onClicked:    mainWindow.showIndicatorDrawer(drawerComponent, control)
    }

    Component {
        id: drawerComponent

        ToolIndicatorPage {
            showExpand:        false
            waitForParameters: false
            contentComponent:  flightModeContentComponent
            // No expandedComponent: the customer drawer has no settings entry point.
        }
    }

    Component {
        id: flightModeContentComponent

        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelWidth / 2

            QGCDelayButton {
                Layout.fillWidth: true
                text:             control._vtolInFWDFlight ? qsTr("Transition to Multi-Rotor") : qsTr("Transition to Fixed Wing")
                visible:          control._isVTOL && control._vehicleInAir

                onActivated: {
                    if (control.activeVehicle) {
                        control.activeVehicle.vtolInFwdFlight = !control._vtolInFWDFlight
                        mainWindow.closeIndicatorDrawer()
                    }
                }
            }

            Repeater {
                // CustomFirmwarePlugin owns the numeric External 3/4 allowlist and
                // preserves names supplied by the aircraft. Ignore old UI hide lists.
                model: control.activeVehicle ? control.activeVehicle.flightModes : []

                QGCDelayButton {
                    required property string modelData
                    Layout.fillWidth: true
                    text:             modelData
                    delay:            QGroundControl.settingsManager.flightModeSettings.requireModeChangeConfirmation.rawValue ? defaultDelay : 0

                    onActivated: {
                        const vehicle = control.activeVehicle
                        if (vehicle && vehicle.flightModes.indexOf(modelData) !== -1) {
                            vehicle.flightMode = modelData
                            mainWindow.closeIndicatorDrawer()
                        }
                    }
                }
            }

            QGCLabel {
                text:    qsTr("No permitted flight modes available")
                visible: !control.activeVehicle || control.activeVehicle.flightModes.length === 0
            }
        }
    }
}
