import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

//import QGroundControl.SynclairVision

Item {
    id: root

    property int borderWidth: 5
    

    QGCPalette { id: qgcPalette}

    implicitWidth: joystick.width + zoom.width - borderWidth
    implicitHeight: Math.max(joystick.height, zoom.height)
    
    SVMenuStrip {
        id: lockButton
        headerless: true
        anchors.left: zoom.right
        anchors.bottom: parent.bottom

        exclusiveSelection: false
        autoUpdateActiveId: false

        direction: horizontal

        activeIds: SVSettings.svLockedControls ? ["lock"] : []


        model: [
            { 
                id: "lock",
                text: "Lock",
                checkable: true,
                iconSource: "/qmlimages/controls_lock.svg",
                alternateIconSource: "/qmlimages/controls_lock_closed.svg",
                iconActive: SVSettings.svLockedControls,
                enabled: true
            }
        ]

        onItemSelected: (id) => {
                SVSettings.svLockedControls = !SVSettings.svLockedControls
                return
        }
    }

    SVVirtualJoystick {
        id: joystick
        height: 200
        width: height
        x: 0
        y: 0
        borderWidth: root.borderWidth
        enabled: !SVSettings.svLockedControls
    }

    SVZoom {
        id: zoom
        height: 120
        width: height / 2
        x: joystick.width - borderWidth
        y: root.implicitHeight - height
        borderWidth: root.borderWidth
        enabled: !SVSettings.svLockedControls
    }
}