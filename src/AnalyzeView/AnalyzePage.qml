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

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

/// Base view control for all Analyze pages
Item {
    anchors.fill:               parent
    anchors.margins:            ScreenTools.defaultFontPixelWidth

    property alias  pageComponent:      pageLoader.sourceComponent
    property alias  pageName:           pageNameLabel.text
    property alias  pageDescription:    pageDescriptionLabel.text
    property alias  headerComponent:    headerLoader.sourceComponent
    property real   availableWidth:     width  - pageLoader.x
    property real   availableHeight:    height - pageLoader.y
    property bool   poped:              false
    property real   _margins:           ScreenTools.defaultFontPixelHeight * 0.5

    signal popout()

    Loader {
        id:                     headerLoader
        anchors.topMargin:      _margins
        anchors.top:            parent.top
        anchors.left:           parent.left
        anchors.right:          floatIcon.left
    }

    Column {
        id:                     headingColumn
        anchors.topMargin:      _margins
        anchors.top:            parent.top
        anchors.left:           parent.left
        anchors.right:          floatIcon.left
        spacing:                _margins
        visible:                !ScreenTools.isShortScreen && headerLoader.sourceComponent === null
        QGCLabel {
            id:                 pageNameLabel
            font.pointSize:     ScreenTools.largeFontPointSize
        }
        QGCLabel {
            id:                 pageDescriptionLabel
            anchors.left:       parent.left
            anchors.right:      parent.right
            wrapMode:           Text.WordWrap
        }
    }

    QGCFlickable {
        anchors.topMargin:      ScreenTools.defaultFontPixelHeight
        anchors.top:            headerLoader.sourceComponent === null ? (headingColumn.visible ? headingColumn.bottom : parent.top) : headerLoader.bottom
        anchors.bottom:         parent.bottom
        anchors.left:           parent.left
        anchors.right:          parent.right
        contentWidth:           pageLoader.x + pageLoader.item.width
        contentHeight:          pageLoader.y + pageLoader.item.height
        clip:                   true
        Loader {
            id:                 pageLoader
        }
    }

    QGCColoredImage {
        id:                     floatIcon
        anchors.top:            parent.top
        anchors.right:          parent.right
        width:                  ScreenTools.defaultFontPixelHeight * 2
        height:                 width
        sourceSize.width:       width
        source:                 "/qmlimages/FloatingWindow.svg"
        fillMode:               Image.PreserveAspectFit
        color:                  qgcPal.text
        visible:                !poped && !ScreenTools.isMobile
        MouseArea {
            anchors.fill: parent
            onClicked: {
                popout()
            }
        }
    }

}
