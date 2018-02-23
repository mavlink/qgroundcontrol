import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQml                    2.2

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Airmap            1.0
import QGroundControl.SettingsManager   1.0

Item {
    id:             _root
    height:         checked ? (header.height + content.height) : header.height
    property var    rules:      null
    property color  color:      "white"
    property alias  text:       title.text
    property bool   checked:    false
    property ExclusiveGroup exclusiveGroup: null
    onExclusiveGroupChanged: {
        if (exclusiveGroup) {
            exclusiveGroup.bindCheckable(_root)
        }
    }
    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }
    Rectangle {
        id:                 header
        height:             ScreenTools.defaultFontPixelHeight * 2
        color:              qgcPal.windowShade
        anchors.top:        parent.top
        anchors.right:      parent.right
        anchors.left:       parent.left
    }
    Row {
        spacing:            ScreenTools.defaultFontPixelWidth * 2
        anchors.fill:       header
        Rectangle {
            height:         parent.height
            width:          ScreenTools.defaultFontPixelWidth * 0.75
            color:          _root.color
        }
        QGCLabel {
            id:                 title
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    QGCColoredImage {
        source:                 checked ? "qrc:/airmap/colapse.svg" : "qrc:/airmap/expand.svg"
        height:                 ScreenTools.defaultFontPixelHeight
        width:                  height
        color:                  qgcPal.text
        fillMode:               Image.PreserveAspectFit
        sourceSize.height:      height
        anchors.right:          header.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter: header.verticalCenter
    }
    MouseArea {
        anchors.fill:       header
        onClicked: {
            _root.checked = !_root.checked
        }
    }
    Rectangle {
        id:             content
        color:          qgcPal.window
        visible:        checked
        height:         ScreenTools.defaultFontPixelHeight * 10
        anchors.top:    header.bottom
        anchors.right:  parent.right
        anchors.left:   parent.left
        anchors.margins: ScreenTools.defaultFontPixelWidth
        Flickable {
            clip:           true
            anchors.fill:   parent
            contentHeight:  rulesetCol.height
            flickableDirection: Flickable.VerticalFlick
            Column {
                id:         rulesetCol
                spacing:    ScreenTools.defaultFontPixelHeight * 0.25
                anchors.right: parent.right
                anchors.left:  parent.left
                Repeater {
                    model:    _root.rules ? _root.rules : []
                    delegate: Item {
                        height:         ruleCol.height
                        anchors.right:  parent.right
                        anchors.left:   parent.left
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        Column {
                            id:         ruleCol
                            spacing:    ScreenTools.defaultFontPixelHeight * 0.5
                            anchors.right:  parent.right
                            anchors.left:   parent.left
                            Item { width: 1; height: ScreenTools.defaultFontPixelHeight * 0.25; }
                            QGCLabel {
                                text:           object.shortText !== "" ? object.shortText : qsTr("Rule")
                                anchors.right:  parent.right
                                anchors.left:   parent.left
                                wrapMode:       Text.WordWrap
                            }
                            QGCLabel {
                                text:           object.description
                                visible:        object.description !== ""
                                font.pointSize: ScreenTools.smallFontPointSize
                                anchors.right:  parent.right
                                anchors.left:   parent.left
                                wrapMode:       Text.WordWrap
                                anchors.rightMargin: ScreenTools.defaultFontPixelWidth * 0.5
                                anchors.leftMargin:  ScreenTools.defaultFontPixelWidth * 0.5
                            }
                        }
                    }
                }
                Item { width: 1; height: ScreenTools.defaultFontPixelHeight * 0.25; }
            }
        }
    }
}
