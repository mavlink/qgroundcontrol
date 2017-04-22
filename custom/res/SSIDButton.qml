/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Item {
    id:         _root
    height:     ssidLabel.height * 3
    width:      ScreenTools.defaultFontPixelWidth * 36
    clip:       true

    property alias          source:         icon.source
    property alias          text:           ssidLabel.text
    property alias          enabled:        mouseArea.enabled
    property bool           checked:        false
    property bool           showIcon:       false
    property bool           rotateImage:    false
    property real           rssi:           0

    property ExclusiveGroup exclusiveGroup:  null

    signal clicked()

    onExclusiveGroupChanged: {
        if (exclusiveGroup) {
            exclusiveGroup.bindCheckable(_root)
        }
    }

    QGCPalette { id: qgcPal }

    Rectangle {
        id:         ssidRect
        height:     parent.height
        width:      parent.width
        color:      checked ? qgcPal.buttonHighlight : qgcPal.button
        Row {
            spacing:        ScreenTools.defaultFontPixelWidth
            anchors.left:       parent.left
            anchors.leftMargin: ScreenTools.defaultFontPixelWidth
            anchors.verticalCenter: parent.verticalCenter
            SignalStrength {
                anchors.verticalCenter: parent.verticalCenter
                size:                   ssidRect.height * 0.5
                percent: {
                    if(rssi < 0) {
                        if(rssi >= -50)
                            return 100;
                        if(rssi <= -100)
                            return 0;
                        return 2 * (rssi + 100);
                    }
                    return 0;
                }
            }
            QGCLabel {
                id:                 ssidLabel
                color:              checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        QGCColoredImage {
            id:                 icon
            height:             ssidLabel.height
            width:              height
            sourceSize.width:   width
            fillMode:           Image.PreserveAspectFit
            color:              checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
            visible:            showIcon
            anchors.right:      parent.right
            anchors.rightMargin: ScreenTools.defaultFontPixelWidth
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id:             mouseArea
        anchors.fill:   parent
        onClicked: {
            checked = true
            _root.clicked()
        }
    }
}
