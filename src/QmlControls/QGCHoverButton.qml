import QtQuick 2.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

// TODO: Use QT styles. Use default button style + custom style entries
import QGroundControl.ScreenTools 1.0
import QGroundControl.Palette 1.0

// TODO: use QT palette
Button {
    id: button
    width: columnItem.contentWidth + contentLayoutItem.margins * 2
    height: width
    flat: true

    property color borderColor: qgcPal.windowShadeDark

    property alias radius: buttonBkRect.radius
    property alias fontPointSize: innerText.font.pointSize
    property alias imageSource: innerImage.source
    property alias contentWidth: innerText.contentWidth

    property real  imageScale: 0.8
    property real  borderWidth: 0
    property real  contentMargins: innerText.height * 0.1

    property color _currentColor: qgcPal.button
    property color _currentContentColor: qgcPal.buttonText

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
        visible: !flat

        border.width: borderWidth
        border.color: borderColor
    }

    // Change the colors based on button states
    states: [
        State {
            name: "Hovering"
            PropertyChanges {
                target: button;
                _currentColor: (checked || pressed) ? qgcPal.buttonHighlight : qgcPal.hoverColor
                _currentContentColor: qgcPal.buttonHighlightText
            }
            PropertyChanges {
                target: buttonBkRect
                visible: true
            }
        },
        State {
            name: "Default"
            PropertyChanges {
                target: button;
                _currentColor: enabled ? ((checked || pressed) ? qgcPal.buttonHighlight : qgcPal.button) : qgcPalDisabled.button
                _currentContentColor: enabled ? ((checked || pressed) ? qgcPal.buttonHighlightText : qgcPal.buttonText) : qgcPalDisabled.buttonText
            }
            PropertyChanges {
                target: buttonBkRect
                visible: !flat || (checked || pressed)
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
