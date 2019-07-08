import QtQuick 2.4
import QtQuick.Shapes 1.11
import QtGraphicalEffects 1.0

Item {
    id: _root
    property color mainColor: "#0B1420"
    property color secondaryColor: "#222A35"
    property color stickColor: "#CCCCCC"
    property int returnAnimationDurationMs: 0;
    property real sideSize: 100
    property real darkerBorders: 1.4

    property real xUnitVal: (stickItem.xC - stickAreaItem._stickCenterPos.x)/(stickAreaItem._movementRadius)
    property real yUnitVal: (stickItem.yC - stickAreaItem._stickCenterPos.y)/(stickAreaItem._movementRadius)

    signal stickLimited();

    width: stickAreaItem.width
    height: stickAreaItem.height

    Rectangle {
        id: stickAreaItem

        width: _root.sideSize
        height: width
        anchors.left: parent.left
        anchors.top: parent.top
        z: 0 // At the bottom
        radius: width/2

        border.color: Qt.darker(color, _root.darkerBorders)
        border.width: parent.width * (1/116)

        color: mainColor

        property var _stickCenterPos: Qt.point(width/2, height/2)
        property real _movementRadius: (width/2) - stickItem.r

        MouseArea {
            id: dragArea
            anchors.fill: parent

            property var _lastMousePress: null

            onPressed: {
                var pressedPoint = mapToItem(stickItem, mouse.x, mouse.y)
                if(contains(pressedPoint)) {
                    recenterAnimation.stop();
                    _lastMousePress = Qt.point(mouse.x, mouse.y);
                    // Update stick
                    onPositionChanged(mouse);
                }
                else {
                    mouse.accepted = false;
                }
            }

            onReleased: {
                _lastMousePress = null
                if(_root.returnAnimationDurationMs > 0) {
                    recenterAnimation.start();
                }
                else {
                    stickItem.xC = stickAreaItem._stickCenterPos.x;
                    stickItem.yC = stickAreaItem._stickCenterPos.y;
                }
            }

            onPositionChanged: {
                if(_lastMousePress !== null) {
                    var distFromCenter = Math.hypot(stickAreaItem._stickCenterPos.x - mouse.x, stickAreaItem._stickCenterPos.y - mouse.y);
                    if(distFromCenter < stickAreaItem._movementRadius) {
                        stickItem.xC = mouse.x;
                        stickItem.yC = mouse.y;
                    }
                    else {
                        var angle = Math.atan2(stickAreaItem._stickCenterPos.x - mouse.x, stickAreaItem._stickCenterPos.y - mouse.y);
                        var maxX = Math.cos(angle) * stickAreaItem._movementRadius;
                        var maxY = Math.sin(angle) * stickAreaItem._movementRadius;
                        stickItem.xC = stickAreaItem._stickCenterPos.x - maxY;
                        stickItem.yC = stickAreaItem._stickCenterPos.y - maxX;
                        _root.stickLimited();
                    }
                }
            }
        }

        Rectangle {
            id: stickItem

            width: parent.width * 36.0/116.0
            height: width
            radius: width/2
            x: xC - width/2
            y: yC - height/2
            property real xC: parent._stickCenterPos.x
            property real yC: parent._stickCenterPos.y
            property real r: width/2
            z: 10 // above all

            border.color: _root.stickColor
            border.width: parent.width * (1/116)

            RadialGradient {
                anchors.centerIn: parent
                width: parent.width - parent.border.width
                height: parent.height - parent.border.width
                gradient: Gradient{
                    GradientStop {
                        position: 0.0
                        color: _root.stickColor
                    }
                    GradientStop {
                        position: 1.0
                        color: Qt.darker(_root.stickColor, _root.darkerBorders)
                    }
                }
                source: stickItem
            }
        }

        Rectangle {
            width: parent.width * 56.0/116.0
            anchors.centerIn: stickAreaItem
            z: 1 // on top of stickAreaItem

            height: width
            radius: width/2

            color: secondaryColor
        }

        ParallelAnimation {
            id: recenterAnimation
            NumberAnimation {
                id: xRecenterNumberAnimation
                target: stickItem;
                property: "xC";
                to: stickAreaItem._stickCenterPos.x;
                duration: _root.returnAnimationDurationMs;
                easing.type: Easing.OutSine
            }
            NumberAnimation {
                target: xRecenterNumberAnimation.target;
                property: "yC";
                to: stickAreaItem._stickCenterPos.y;
                duration: xRecenterNumberAnimation.duration;
                easing.type: xRecenterNumberAnimation.easing.type
            }
        }
    }
}
