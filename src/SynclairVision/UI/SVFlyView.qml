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

    property int _widgetMargin: 0
    property int _toolBarHeight: 0

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

    SVCameraLayer {
        id: cameraLayer
        anchors.left: parent.left
        anchors.top: parent.top
        width: parent.width / 2
        height: parent.height / 2
        parentToolInsets: root.parentToolInsets
        _widgetMargin: root._widgetMargin
    }

    SVFlyViewWidgetLayer {
        id: widgetLayer
        anchors.fill: parent
        anchors.margins: _widgetMargin
        anchors.topMargin: _widgetMargin + _toolBarHeight
        parentToolInsets: root.parentToolInsets

    }

    
}
