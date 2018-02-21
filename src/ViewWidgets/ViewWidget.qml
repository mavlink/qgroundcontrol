import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0

Rectangle {
    property Component connectedComponent: __componentConnected
    property Component disconnectedComponent: __componentDisconnected

    QGCPalette { id: __qgcPal; colorGroupEnabled: enabled }
    ViewWidgetController { id: __controller }

    color: __qgcPal.window

    Component.onCompleted: __controller.checkForVehicle()

    Connections {
        target: __controller

        onPluginConnected: {
            pageLoader.autopilot = autopilot
            pageLoader.sourceComponent = connectedComponent
        }

        onPluginDisconnected: {
            pageLoader.sourceComponent = null
            pageLoader.sourceComponent = disconnectedComponent
            pageLoader.autopilot = null
        }
    }

    Loader {
        id: pageLoader

        anchors.fill: parent

        property var autopilot

        sourceComponent: __componentDisconnected
    }

    Component {
        id: __componentConnected

        Rectangle {
            QGCPalette { id: __qgcPal; colorGroupEnabled: enabled }

            anchors.fill:	parent
            color:			__qgcPal.window

            QGCLabel {
                anchors.fill:	parent

                horizontalAlignment:	Text.AlignHCenter
                verticalAlignment:		Text.AlignVCenter

                text: qsTr("missing connected implementation")
            }
        }
    }

    Component {
        id: __componentDisconnected

        Rectangle {
            QGCPalette { id: __qgcPal; colorGroupEnabled: enabled }

            anchors.fill:	parent
            color:			__qgcPal.window

            QGCLabel {
                anchors.fill:	parent

                horizontalAlignment:	Text.AlignHCenter
                verticalAlignment:		Text.AlignVCenter

                text: qsTr("no vehicle connected")
            }
        }
    }
}
