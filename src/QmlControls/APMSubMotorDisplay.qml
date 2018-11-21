import QtQuick 2.3

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
Item {
    id: root

    property var frameType: 0
    // TODO need a better class for getting vehicle parameters into qml?
    // according to comments in FactPanelController.h, this is not the intended use case

    function getImage() {
        switch (frameType) {
        case 0:
                return "qrc:///qmlimages/Frames/BlueROV1.png"
        case 1:
                return "qrc:///qmlimages/Frames/Vectored.png"
        case 2:
                return "qrc:///qmlimages/Frames/Vectored6DOF.png"
        case 4:
                return "qrc:///qmlimages/Frames/SimpleROV-3.png"
        case 5:
                return "qrc:///qmlimages/Frames/SimpleROV-4.png"
        }
        return ""
    }

    Component.onCompleted: {
        console.log(getImage())
        subImage.source = getImage()
    }

    Image {
        id: subImage
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.fill:       parent
        fillMode:           Image.PreserveAspectFit
        smooth:             true
        mipmap:             true
    }
} // Item
