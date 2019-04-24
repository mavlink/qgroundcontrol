import QtQuick 2.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

// TODO: Use QT styles. Use default button style + custom style entries
import QGroundControl.ScreenTools 1.0
import QGroundControl.Palette 1.0

Button {
    id: button
    width: columnItem.contentWidth + contentLayoutItem.margins * 2
    height: width
    flat: true

    property color color: qgcPal.button
    property color disabledColor: qgcPalDisabled.button
    property color pressedColor: qgcPal.buttonHighlight
    // TODO: remove after we add it to the palette
    property color hoverColor: qgcPal.hoverColor
    property color contentColor: qgcPal.buttonText
    property color contentPressedColor: qgcPal.buttonHighlightText
    property color borderColor: qgcPal.windowShadeDark

    property alias radius: buttonBkRect.radius
    property alias fontPointSize: innerText.font.pointSize
    property alias imageSource: innerImage.source
    property alias contentWidth: innerText.contentWidth

    property real  imageScale: 0.8
    property real  borderWidth: 0
    property real  contentMargins: innerText.height * 0.1

    property color _currentColor: checked ? pressedColor : color
    property color _currentContentColor: contentColor

    QGCPalette { id: qgcPal }
    QGCPalette { id: qgcPalDisabled; colorGroupEnabled: false }

    // Initial state
    state:         "Default"
    // Update state on status changed
    onEnabledChanged: state = "Default"

    property real _contentVDist: innerImage.height/innerText.contentHeight

    // Content Icon + Text
    contentItem: Item {
        id: contentLayoutItem
        anchors.fill: parent
        anchors.margins: contentMargins

        Column {
            id: columnItem
            anchors.fill: parent

            Item {
                width: parent.width
                height: (contentLayoutItem.height - innerText.height)
                Image {
                    id: innerImage

                    anchors.centerIn: parent

                    height: parent.height * imageScale
                    width: parent.width * imageScale

                    visible: false
                    smooth: true
                    antialiasing: true
                    mipmap: true
                    fillMode: Image.PreserveAspectFit
                    sourceSize.height: height
                    sourceSize.width: width
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                }

                ColorOverlay {
                    id: imageOverlay
                    anchors.fill: innerImage
                    source: innerImage

                    color: _currentContentColor
                }
            }

            Text {
                id: innerText

                text: button.text
                color: _currentContentColor
                width: parent.width

                font.pointSize: ScreenTools.defaultFontPointSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        } // Column - content
    } // Item - content

    background: Rectangle {
        id: buttonBkRect
        anchors.fill: parent
        color: _currentColor

        border.width: borderWidth
        border.color: borderColor
    }

    // Change the aspect of the content in differen button states
    states: [
        State {
            name: "Hovering"
            PropertyChanges {
                target: button;
                _currentColor: pressed || checked ? pressedColor : hoverColor
                _currentContentColor: pressed || checked ? contentPressedColor : contentColor
            }
        },
        State {
            name: "Default"
            PropertyChanges {
                target: button;
                _currentColor: enabled ? ((checked || pressed) ? pressedColor : color) : disabledColor
                _currentContentColor: contentColor
            }
        }
    ]

    transitions: [
        Transition {
            from: ""; to: "Hovering"
            ColorAnimation { duration: 200 }
        },
        Transition {
            from: "*"; to: "Pressed"
            ColorAnimation { duration: 10 }
        }
    ]

    // Process hover events
    MouseArea {
        enabled: !ScreenTools.isMobile
        hoverEnabled: true
        propagateComposedEvents: true
        preventStealing: true
        anchors.fill: button
        onEntered: { button.state = 'Hovering'; }
        onExited: { button.state = 'Default'; }
        // Propagate events down
        onClicked: { mouse.accepted = false; }
        onDoubleClicked: { mouse.accepted = false; }
        onPositionChanged: { mouse.accepted = false; }
        onPressAndHold: { mouse.accepted = false; }
        onPressed: { mouse.accepted = false }
        onReleased: { mouse.accepted = false }
    }
}
