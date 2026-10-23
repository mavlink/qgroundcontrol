import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

/// Column of tab pills, styled like QGCGroupBox header buttons. Pills show vertical text when there is room and icons
/// otherwise. The right side of each pill is padding meant to sit under the panel it is attached to.
Column {
    id: root

    property var    model:          []  ///< Entries from PlanPanelLogic.tabModel()
    property string currentTab
    property string style:          "icon"  ///< "text" or "icon"

    signal tabClicked(string tab)

    readonly property bool _textStyle: style === "text"
    readonly property var _icons: ({
        "plan":     "/qmlimages/Plan.svg",
        "mission":  "/res/waypoint.svg",
        "fence":    "/res/GeoFence.svg",
        "rally":    "/res/RallyPoint.svg"
    })
    readonly property var _labels: ({
        "plan":     qsTr("Plan"),
        "mission":  qsTr("Mission"),
        "fence":    qsTr("Fence"),
        "rally":    qsTr("Rally")
    })

    spacing: ScreenTools.defaultFontPixelWidth / 2

    Repeater {
        model: root.model

        AbstractButton {
            id: pill

            required property var modelData

            readonly property bool current: modelData.id === root.currentTab

            objectName:     "planPanel_tab_" + modelData.id
            text:           root._labels[modelData.id]
            implicitWidth:  ScreenTools.defaultFontPixelHeight * 2.2
            implicitHeight: root._textStyle ? ScreenTools.defaultFontPixelHeight * 1.5 + tabText.implicitWidth : ScreenTools.defaultFontPixelHeight * 2.6
            rightPadding:   ScreenTools.defaultBorderRadius * 2
            hoverEnabled:   !ScreenTools.isMobile
            focusPolicy:    Qt.ClickFocus

            Accessible.name: text

            onClicked: root.tabClicked(modelData.id)

            QGCPalette { id: pillPal }

            background: Rectangle {
                radius:         ScreenTools.defaultBorderRadius
                color:          pill.current ? pillPal.accentSubtle : pillPal.windowPanel
                border.width:   1
                border.color:   pill.current || pill.hovered || pill.pressed ? pillPal.accent : pillPal.borderSubtle
            }

            contentItem: Item {
                QGCColoredImage {
                    anchors.centerIn:       parent
                    width:                  ScreenTools.defaultFontPixelHeight
                    height:                 width
                    visible:                !root._textStyle
                    source:                 root._icons[pill.modelData.id]
                    color:                  pillPal.textPrimary
                    fillMode:               Image.PreserveAspectFit
                    sourceSize.height:      height
                }

                QGCLabel {
                    id:                 tabText
                    anchors.centerIn:   parent
                    rotation:           -90
                    visible:            root._textStyle
                    text:               pill.text
                    color:              pillPal.textPrimary
                }
            }

            // Warning dot at the top-left corner of the pill, overlapping the icon or the end of the text a little
            Rectangle {
                id: badge

                readonly property real margin: ScreenTools.defaultFontPixelHeight * 0.25

                objectName:         "planPanel_tabBadge_" + pill.modelData.id
                x:                  margin
                y:                  margin
                z:                  1
                width:              ScreenTools.defaultFontPixelHeight * 0.6
                height:             width
                radius:             width / 2
                visible:            pill.modelData.warn
                color:              pillPal.colorOrange
            }
        }
    }
}
