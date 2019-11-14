import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Layouts          1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Rectangle {
    id:     _root
    height: pageFlickable.y + pageFlickable.height + _margins
    color:  qgcPal.window
    radius: ScreenTools.defaultFontPixelWidth * 0.5

    property real   maxHeight       ///< Maximum height that should be taken, smaller than this is ok

    property real   _margins:           ScreenTools.defaultFontPixelWidth / 2
    property real   _pageWidth:         _root.width
    property var    _instrumentPages:   QGroundControl.corePlugin.instrumentPages

    QGCPalette { id:qgcPal; colorGroupEnabled: parent.enabled }

    QGCComboBox {
        id:             pageCombo
        anchors.left:   parent.left
        anchors.right:  parent.right
        model:          _instrumentPages
        textRole:       "title"
        centeredLabel:  true
        font.pointSize: ScreenTools.smallFontPointSize

        QGCColoredImage {
            anchors.leftMargin:     _margins
            anchors.left:           parent.left
            anchors.verticalCenter: parent.verticalCenter
            source:                 "/res/gear-black.svg"
            mipmap:                 true
            height:                 parent.height * 0.7
            width:                  height
            sourceSize.height:      height
            color:                  qgcPal.text
            fillMode:               Image.PreserveAspectFit
            visible:                pageWidgetLoader.item ? (pageWidgetLoader.item.showSettingsIcon ? pageWidgetLoader.item.showSettingsIcon : false) : false

            QGCMouseArea {
                fillItem:   parent
                onClicked:  pageWidgetLoader.item.showSettings()
            }
        }
    }

    QGCFlickable {
        id:                 pageFlickable
        anchors.margins:    _margins
        anchors.top:        pageCombo.bottom
        anchors.left:       parent.left
        anchors.right:      parent.right
        height:             Math.min(_maxHeight, pageWidgetLoader.height)
        contentHeight:      pageWidgetLoader.height
        flickableDirection: Flickable.VerticalFlick
        clip:               true

        property real _maxHeight: maxHeight - y - _margins

        Loader {
            id:     pageWidgetLoader
            source: _instrumentPages[pageCombo.currentIndex].url
            property real pageWidth:  parent.width
        }
    }
}
