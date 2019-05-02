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
    property real   availableWidth:     width  - pageLoader.x
    property real   availableHeight:    height - pageLoader.y
    property real   _margins:           ScreenTools.defaultFontPixelHeight * 0.5

    QGCFlickable {
        anchors.fill:           parent
        contentWidth:           pageLoader.x + pageLoader.item.width
        contentHeight:          pageLoader.y + pageLoader.item.height
        clip:                   true
        Column {
            id:                 headingColumn
            width:              parent.width
            spacing:            _margins
            QGCLabel {
                id:             pageNameLabel
                font.pointSize: ScreenTools.largeFontPointSize
                visible:        !ScreenTools.isShortScreen
            }
            QGCLabel {
                id:             pageDescriptionLabel
                anchors.left:   parent.left
                anchors.right:  parent.right
                wrapMode:       Text.WordWrap
                visible:        !ScreenTools.isShortScreen
            }
        }
        Loader {
            id:                 pageLoader
            anchors.topMargin:  _margins
            anchors.top:        headingColumn.bottom
        }
    }
}
