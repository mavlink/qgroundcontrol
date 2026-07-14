import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models
import QtQuick.Shapes 2.15
import "../Camera/SVCameraLayouts.js" as SVCameraLayouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

Item {
    id: root

    property var parentToolInsets
    property real leftToolStripBottom: 0

    property int _widgetMargin: 0
    property int _toolBarHeight: 0

    readonly property var cameraLayouts: SVCameraLayouts.getCameraLayouts()
    readonly property var activeLayout: {
        for (let i = 0; i < cameraLayouts.length; i++) {
            if (cameraLayouts[i].id === SVState.layout) {
                return cameraLayouts[i]
            }
        }

        return cameraLayouts.length > 0 ? cameraLayouts[0] : null
    }
    readonly property string resolvedActiveLayoutId: activeLayout ? activeLayout.id : ""

    QGCPalette { id: qgcPalette}


    Repeater {
        model: root.activeLayout ? root.activeLayout.panes : []

        delegate: SVCameraLayer {
            required property var modelData
            required property int index

            width: modelData.w * root.width
            height: modelData.h * root.height
            x: modelData.x * root.width
            y: modelData.y * root.height
            cameraIndex: index
            
            _widgetMargin: root._widgetMargin
        }
    }

    Item {
        id: separatorLayer
        anchors.fill: parent
        z: 1

        Repeater {
            model: root.activeLayout ? root.activeLayout.separators : []

            delegate: Rectangle {
                required property var modelData
                readonly property bool isVertical: modelData.orientation === 'vertical'
                readonly property bool isHorizontal: modelData.orientation === 'horizontal'

                width:  isVertical   ? SVUnits.lineWidth : modelData.length * root.width
                height: isHorizontal ? SVUnits.lineWidth : modelData.length * root.height 
                x: modelData.x * root.width - (isVertical ? width / 2 : 0)
                y: modelData.y * root.height - (isHorizontal ? height / 2: 0)


                color: qgcPalette.windowShade
            }
        }
    }

    SVFlyViewWidgetLayer {
        id: widgetLayer
        z: 2
        anchors.fill: parent
        anchors.margins: _widgetMargin
        anchors.topMargin: _widgetMargin + _toolBarHeight
        leftToolStripBottom: root.leftToolStripBottom
        activeLayoutId: root.resolvedActiveLayoutId
        onLayoutSelected: (layoutId) => SVState.layout = layoutId
    }

    

    Rectangle {
        id: recordBorder
        z: 100
        anchors.fill: parent
        color: "transparent"
        border.width: SVUnits.thickLineWidth + SVUnits.lineWidth * 2
        border.color: qgcPalette.colorRed
        visible: SVState.record

        SequentialAnimation on opacity {
            loops: Animation.Infinite
            running: true

            NumberAnimation { from: 1.0; to: 0.4; duration: 1000; easing.type: Easing.InOutSine }
            NumberAnimation { from: 0.4; to: 1.0; duration: 1000; easing.type: Easing.InOutSine }
        }
    }

    Rectangle {
        id: photoBorder
        z: 101
        anchors.fill: parent
        color: "transparent"
        border.width: SVUnits.thickLineWidth * 4
        border.color: "white"
        visible: opacity > 0.01

        SequentialAnimation on opacity {
            loops: 1
            running: true

            NumberAnimation { from: 1.0; to: 0.0; duration: 1000; easing.type: Easing.InOutSine }
            
        }
    }

    
}
