import QtQuick          2.3
import QtQuick.Controls 1.2

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

/// File Button controls used by QGCFileDialog control
Rectangle {
    implicitWidth:  ScreenTools.implicitButtonWidth
    implicitHeight: ScreenTools.implicitButtonHeight
    color:          highlight ? qgcPal.buttonHighlight : qgcPal.button
    border.color:   highlight ? qgcPal.buttonHighlightText : qgcPal.buttonText

    property alias  text:       label.text
    property bool   highlight:  false

    signal clicked
    signal hamburgerClicked

    property real _margins: ScreenTools.defaultFontPixelWidth / 2

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    QGCLabel {
        id:                     label
        anchors.margins:         _margins
        anchors.left:           parent.left
        anchors.right:          hamburger.left
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        verticalAlignment:      Text.AlignVCenter
        horizontalAlignment:    Text.AlignHCenter
        color:                  highlight ? qgcPal.buttonHighlightText : qgcPal.buttonText
        elide:                  Text.ElideRight
    }

    QGCColoredImage {
        id:                     hamburger
        anchors.rightMargin:    _margins
        anchors.right:          parent.right
        anchors.verticalCenter: parent.verticalCenter
        width:                  _hamburgerSize
        height:                 _hamburgerSize
        sourceSize.height:      _hamburgerSize
        source:                 "qrc:/qmlimages/Hamburger.svg"
        color:                  highlight ? qgcPal.buttonHighlightText : qgcPal.buttonText

        property real _hamburgerSize: parent.height * 0.75
    }

    QGCMouseArea {
        anchors.fill:   parent
        onClicked:      parent.clicked()
    }

    QGCMouseArea {
        anchors.leftMargin: -_margins * 2
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.right:      parent.right
        anchors.left:       hamburger.left
        onClicked:          parent.hamburgerClicked()
    }
}
