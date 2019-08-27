import QtQuick 2.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

// TODO: Use QT styles. Use default button style + custom style entries
import QGroundControl.ScreenTools 1.0
import QGroundControl.Palette 1.0

// TODO: use QT palette
Button {
    id:     button
    width:  contentLayoutItem.contentWidth + (contentMargins * 2)
    height: width
    flat:   true

    property color borderColor:     qgcPal.windowShadeDark

    property alias radius:          buttonBkRect.radius
    property alias fontPointSize:   innerText.font.pointSize
    property alias imageSource:     innerImage.source
    property alias contentWidth:    innerText.contentWidth

    property real  imageScale:      0.6
    property real  borderWidth:     0
    property real  contentMargins: innerText.height * 0.1

    property color _currentColor:           qgcPal.button
    property color _currentContentColor:    qgcPal.buttonText

    QGCPalette { id: qgcPalDisabled; colorGroupEnabled: false }

    // Initial state
    state:                  "Default"
    // Update state on status changed
    // Content Icon + Text
    contentItem: Item {
        id:                 contentLayoutItem
        anchors.fill:       parent
        anchors.margins:    contentMargins
        Column {
            anchors.centerIn:   parent
            spacing:        contentMargins * 2
            QGCColoredImage {
                id:         innerImage
                height:     contentLayoutItem.height * imageScale
                width:      contentLayoutItem.width  * imageScale
                smooth:     true
                mipmap:     true
                color:      _currentContentColor
                fillMode:   Image.PreserveAspectFit
                antialiasing: true
                sourceSize.height:  height
                sourceSize.width:   width
                anchors.horizontalCenter: parent.horizontalCenter
            }
            QGCLabel {
                id:         innerText
                text:       button.text
                color:      _currentContentColor
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    background: Rectangle {
        id:                 buttonBkRect
        color:              _currentColor
        visible:            !flat
        border.width:       borderWidth
        border.color:       borderColor
        anchors.fill:       parent
    }

    // Change the colors based on button states
    states: [
        State {
            name: "Hovering"
            PropertyChanges {
                target: button;
                _currentColor:          (checked || pressed) ? qgcPal.buttonHighlight : qgcPal.hoverColor
                _currentContentColor:   qgcPal.buttonHighlightText
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
                _currentColor:          enabled ? ((checked || pressed) ? qgcPal.buttonHighlight : qgcPal.button) :         qgcPalDisabled.button
                _currentContentColor:   enabled ? ((checked || pressed) ? qgcPal.buttonHighlightText : qgcPal.buttonText) : qgcPalDisabled.buttonText
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
        enabled:            !ScreenTools.isMobile
        propagateComposedEvents: true
        hoverEnabled:       true
        preventStealing:    true
        anchors.fill:       button
        onEntered:          button.state = 'Hovering'
        onExited:           button.state = 'Default'
        // Propagate events down
        onClicked:          { mouse.accepted = false; }
        onDoubleClicked:    { mouse.accepted = false; }
        onPositionChanged:  { mouse.accepted = false; }
        onPressAndHold:     { mouse.accepted = false; }
        onPressed:          { mouse.accepted = false }
        onReleased:         { mouse.accepted = false }
    }
}
