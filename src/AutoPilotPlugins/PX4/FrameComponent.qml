import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

SetupPage {
    id: framePage
    // This is what the loader 'loads'
    pageComponent: framePageComponent

    // Master controller for the frames interface
    property var framesBase:        globals.activeVehicle.framesBase
    // Frames tree structure root node
    //property var frames:            framesBase.rootFrame

    // Component that will be loaded via `SetupPage`
    Component {
        id: framePageComponent

        // Toolbar / Debug console
        Row {
            id: toolbar
            width: parent.width; height: parent.height / 10 // Hacky

            Button {
                id: gotoParentButton
                text: "Go to parent"
                onClicked: {
                    framesBase.gotoParentFrame();
                }
            }

            // Frames collage view
            Flow {
                id: framesCollageView
                width: parent.width
                //spacing: _boxSpacing

                Repeater {
                    id: framesRepeater
                    model: framesBase.selectedFrames // show Selected Frames only

                    PX4Frame {
                        id: frameId
                        frame: object // why do we call it 'object', not 'modelData'?
                        selected: frame.frame_param_values == framesBase.finalSelectionFrameParamValues
                    }
                }
            }
        }
    }
}
