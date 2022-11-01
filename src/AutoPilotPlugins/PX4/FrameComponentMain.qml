import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

import Junwoo.framecomp 1.0

Window {
    id: mainPage
    visible: true
    title: qsTr("Main Page")
    width: 1000; height: 1000

    // Properties
    property real _defaultFontPointSize: 10
    property real _defaultFontPixelWidth: 10
    property real _defaultFontPixelHeight: 10
    property real _boxWidth: _defaultFontPixelWidth * 30
    property real _boxHeight: _defaultFontPixelHeight * 30
    property real _boxSpacing: _defaultFontPixelWidth

    // Reference the Singleton
    property var frameComponent: FrameComponent

    // Background Color
    Rectangle {
        anchors.fill: parent
        objectName: "rect"
        color: "lightblue"
    }

    // Toolbar / Debug console
    Row {
        id: toolbar
        width: parent.width; height: _boxHeight / 4 // Hacky

        Text {
            id: framesParamName
            text: frameComponent.frames_id_param_name
        }

        Button {
            id: gotoParentButton
            text: "Go to parent"
            onClicked: {
                frameComponent.gotoParentFrame();
            }
        }

        Text {
            id: finalSelectedFrameID
            text: frameComponent.finalSelectionFrameID
        }
    }

    // Frames collage view
    Flow {
        id: framesCollageView
        width: parent.width
        anchors.top: toolbar.bottom
        spacing: _boxSpacing

        Repeater {
            id: framesRepeater
            model: frameComponent.selectedFrames

//            onModelChanged: {
//                framesAnimation.running = true
//                console.log('onModelChanged called!')
//            }

//            Connections {
//                target: frameComponent
//                function onSelectedFramesChanged() {
//                    framesAnimation.running = true
//                    console.log('onModelChanged called!')
//                }
//            }

            Frame {
                id: frameId
                frame: modelData
                selected: frame.frame_id == frameComponent.finalSelectionFrameID

                // Click border (don't include the last row)
//                MouseArea {
//                    id: mouseArea
//                    anchors.fill: parent
////                    anchors {
////                        top: parent.top
////                        bottom: frameBottomRow.top
////                        left: parent.left
////                        right: parent.right
////                    }

//                    hoverEnabled: true
//                    onClicked: {
//                        frameComponent.selectFrame(modelData)
//                    }
//                }

//                mouseArea.onClicked: {
//                    frameComponent.selectFrame(modelData)
//                }

//                NumberAnimation {
//                    id: framesAnimation
//                    target: frameId
//                    property: "x"
//                    duration: 1000
//                    to: 0; from: framesCollageView.width
//                    easing.type: Easing.InOutQuad
//                }

//                Component.onCompleted: {
//                    framesAnimation.running = true
//                    console.log('Component.onCompleted called!')
//                }
            }
        }

    }
}
