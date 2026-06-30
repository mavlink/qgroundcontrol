import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models
import QtQuick.Shapes 2.15
import "SVCameraLayouts.js" as SVCameraLayouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

Item {
    id: root

    property var parentToolInsets

    property int _widgetMargin: 0
    property int _toolBarHeight: 0

    property var cameraLayouts: []
    property string activeLayoutId: "four_square"
    property var activeLayout: null
    property real separatorThickness: 1
    property color seperatorColor: qgcPalette.windowShade

    function findLayout(layoutId) {
        for (let i = 0; i < cameraLayouts.length; i++) {
            if (cameraLayouts[i].id === layoutId) {
                return cameraLayouts[i]
            }
        }
        return null
    }

    function updateActiveLayout() {
        activeLayout = findLayout(activeLayoutId)

        if (!activeLayout && cameraLayouts.length > 0) {
            activeLayout = cameraLayouts[0]
            activeLayoutId = activeLayout.id
        }
    }

    function setActiveLayout(layoutId) {
        if (activeLayoutId === layoutId) {
            return
        }

        activeLayoutId = layoutId
        updateActiveLayout()
    }

    Component.onCompleted: {
        cameraLayouts = SVCameraLayouts.getCameraLayouts()
        updateActiveLayout()
    }

    QGCToolInsets {
        id: _toolInsets
        leftEdgeTopInset:       parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset:    parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset:      parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset:   parentToolInsets.rightEdgeCenterInset
        rightEdgeBottomInset:   parentToolInsets.rightEdgeBottomInset
        topEdgeLeftInset:       parentToolInsets.topEdgeLeftInset
        topEdgeCenterInset:     parentToolInsets.topEdgeCenterInset
        topEdgeRightInset:      parentToolInsets.topEdgeRightInset
        bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset:  parentToolInsets.bottomEdgeCenterInset
        bottomEdgeRightInset:   parentToolInsets.bottomEdgeRightInset
    }

    QGCPalette { id: qgcPalette}


    Repeater {
        model: root.activeLayout ? root.activeLayout.panes : []

        delegate: SVCameraLayer {
            required property var modelData

            x: modelData.x * root.width
            y: modelData.y * root.height
            width: modelData.w * root.width
            height: modelData.h * root.height

            parentToolInsets: root.parentToolInsets
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

                readonly property bool isVertical: modelData.orientation === "vertical"
                readonly property bool isHorizontal: modelData.orientation === "horizontal"

                visible: isVertical || isHorizontal
                color: seperatorColor

                width: isVertical ? root.separatorThickness : modelData.length * root.width
                height: isVertical ? modelData.length * root.height : root.separatorThickness
                x: isVertical ? (modelData.x * root.width) - (width / 2) : modelData.x * root.width
                y: isVertical ? modelData.y * root.height : (modelData.y * root.height) - (height / 2)
            }
        }
    }

    SVFlyViewWidgetLayer {
        id: widgetLayer
        z: 2
        anchors.fill: parent
        anchors.margins: _widgetMargin
        anchors.topMargin: _widgetMargin + _toolBarHeight
        parentToolInsets: root.parentToolInsets
    }
}
