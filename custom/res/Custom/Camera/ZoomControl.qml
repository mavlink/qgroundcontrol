import QtQuick 2.0
import QtQuick.Controls 2.4
import QtGraphicalEffects 1.0


Item {
    id: _root

    property color mainColor: "#000000"
    property color contentColor: "#FFFFFF"
    property alias fontPointSize: zoomStatusTextItem.font.pointSize
    property real  zoomLevel: NaN
    property alias zoomLevelVisible: zoomStatusItem.visible
    property bool  showZoomPrecision: true
    property bool  onlyContinousZoom: false

    signal zoomIn()
    signal zoomOut()
    signal continuousZoomStart(var zoomIn)
    signal continuousZoomStop()

    //
    // Beware the buttons were switched
    //
    //

    height: zoomStatusTextItem.height * 2
    width: (zoomLevelVisible ? (zoomStatusItem.width - zoomInButton.width/2) : 0) + zoomInButton.width + zoomOutButton.width

    Rectangle {
        id: zoomStatusItem

        color: mainColor
        opacity: 0.5
        radius: height/2

        anchors.left: _root.left
        anchors.verticalCenter: _root.verticalCenter

        width: height * 2
        height: _root.height * 0.8
    }

    Item {
        visible: zoomStatusItem.visible

        anchors.left: zoomStatusItem.left
        anchors.top: zoomStatusItem.top
        anchors.right: zoomStatusItem.horizontalCenter
        anchors.bottom: zoomStatusItem.bottom

        z: zoomStatusItem.z + 1

        Text {
            id: zoomStatusTextItem

            anchors.centerIn: parent
            opacity: 2

            color: _root.contentColor

            text: isNaN(zoomLevel) ? "-" : "x" + _root.zoomLevel.toFixed(_root.showZoomPrecision ? 1 : 0)
        }
    }

    Button {
        id: zoomInButton
        flat: true

        anchors.left: zoomLevelVisible ? zoomStatusItem.horizontalCenter : _root.left
        anchors.top: _root.top
        width: height
        height: _root.height

        property bool holding: false

        onPressed: {
            if(onlyContinousZoom) {
                holding = true
            }
            else {
                _root.zoomOut()
            }
        }

        onPressAndHold: {
            holding = true
        }

        onReleased: {
            holding = false
        }

        background: Rectangle {
            anchors.fill: zoomInButton
            radius: zoomInButton.width/10

            color: _root.mainColor
        }

        contentItem: Item {
            anchors.fill: zoomInButton
            Rectangle {
                id: zoomInMinusRectangle
                anchors.centerIn: parent

                width: zoomInButton.width * 0.4
                height: zoomInButton.height * 0.05

                color: _root.contentColor
            }
        }
    }

    Item {
        id: buttonSeparator

        anchors.left: zoomInButton.right
        anchors.verticalCenter: zoomInButton.verticalCenter
        width: zoomInButton.width * 0.05
        height: zoomInButton.height * 0.8

        Rectangle {
            radius: width * 0.2
            anchors.centerIn: parent

            width: zoomInButton.width * 0.01
            height: parent.height * 0.8

            color: _root.contentColor
        }
    }

    Button {
        id: zoomOutButton
        flat: true

        anchors.left: buttonSeparator.right
        anchors.top: zoomInButton.top
        width: height
        height: zoomInButton.height

        property bool holding: false

        onPressed: {
            if(onlyContinousZoom) {
                holding = true
            }
            else {
                _root.zoomIn()
            }
        }

        onPressAndHold: {
            holding = true
        }

        onReleased: {
            holding = false
        }

        background: Rectangle {
            anchors.fill: zoomOutButton
            radius: zoomOutButton.width/10

            color: _root.mainColor
        }

        contentItem: Item {
            anchors.fill: zoomOutButton
            Rectangle {
                id: zoomOutMinusRectangle
                anchors.centerIn: parent

                width: zoomInMinusRectangle.width
                height: zoomInMinusRectangle.height

                color: _root.contentColor
            }
            Rectangle {
                anchors.centerIn: parent

                width: zoomOutMinusRectangle.height
                height: zoomOutMinusRectangle.width

                color: _root.contentColor
            }
        }
    }

    // Zoom buttons background
    Rectangle {
        color: _root.mainColor
        z: -1

        anchors.left: zoomInButton.horizontalCenter
        anchors.right: zoomOutButton.horizontalCenter
        anchors.top: zoomInButton.top
        anchors.bottom: zoomOutButton.bottom
    }

    onStateChanged: {
        if(state == "ZoomingIn") {
            _root.continuousZoomStart(true)
        }
        else if(state == "ZoomingOut") {
            _root.continuousZoomStart(false)
        }
        else {
            _root.continuousZoomStop()
        }
    }

    state: "None"
    states: [
        State {
            name: "None"
            when: zoomInButton.holding === false && zoomOutButton.holding === false
        },
        State {
            name: "ZoomingIn"
            when: zoomOutButton.holding === true
        },
        State {
            name: "ZoomingOut"
            when: zoomInButton.holding === true
        }
    ]
}
