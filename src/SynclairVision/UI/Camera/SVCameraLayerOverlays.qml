import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls 

Item {
    id: root

    property bool grid: false
    property bool crosshair: false

    property int crosshairSize: SVUnits.height * 2
    property int crosshairCenter: SVUnits.margin

    property int borderWidth: 1
    property color borderColor: "white"

    Item {
        id: grid
        anchors.fill: parent
        visible: root.grid

        property int stepX: width / 3
        property int stepY: height / 3

        SVLine { thickness: root.borderWidth; color: root.borderColor; startX: 0;         startY: parent.stepY;      endX: parent.width; endY: parent.stepY }
        SVLine { thickness: root.borderWidth; color: root.borderColor; startX: 0;         startY: parent.stepY * 2;  endX: parent.width; endY: parent.stepY * 2 }
        SVLine { thickness: root.borderWidth; color: root.borderColor; startX: parent.stepX;     startY: 0;          endX: parent.stepX;        endY: parent.height }
        SVLine { thickness: root.borderWidth; color: root.borderColor; startX: parent.stepX * 2; startY: 0;          endX: parent.stepX * 2;    endY: parent.height }

        
    }

    Item {
        id: combination
        anchors.fill: parent
        visible: root.crosshair

        SVLine { thickness: root.borderWidth; color: root.borderColor; startX: 0; startY: 0;             endX: parent.width; endY: parent.height }
        SVLine { thickness: root.borderWidth; color: root.borderColor; startX: 0; startY: parent.height; endX: parent.width; endY: 0}
    }

    Item {
        id: crosshair
        width: root.crosshairSize
        height: root.crosshairSize
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        visible: false
        enabled: false

        

        SVLine { startX: 0; startY: parent.height / 2; endX: parent.width / 2 - root.crosshairCenter; endY: parent.height / 2}
        SVLine { startX: parent.width / 2 + root.crosshairCenter; startY: parent.height / 2; endX: parent.width; endY: parent.height / 2}

        SVLine { startX: parent.width / 2; startY: 0; endX: parent.width / 2; endY: parent.height / 2 - root.crosshairCenter}
        SVLine { startX: parent.width / 2; startY: parent.height / 2 + root.crosshairCenter; endX: parent.width / 2; endY: parent.height}

    }
}
