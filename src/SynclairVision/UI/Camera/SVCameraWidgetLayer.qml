import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models
import QtQuick.Shapes 2.15

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

Item {
    id: root

    property var parentToolInsets

    QGCPalette { id: qgcPalette }

    HorizontalCompassAttitude {
            id: compass
            width: 200
            border.width: 1
            border.color: qgcPalette.windowShade
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            gradient: Gradient {
                GradientStop { position: 0.0;  color: qgcPalette.windowTransparent }
                GradientStop { position: 0.90;  color: qgcPalette.windowTransparent }
                GradientStop { position: 1.0;  color: Qt.tint(
                    qgcPalette.windowTransparent,
                    Qt.rgba(
                        qgcPalette.windowShade.r,
                        qgcPalette.windowShade.g,
                        qgcPalette.windowShade.b,
                        0.25
                    )
                ) }
            }
        }
    
}

