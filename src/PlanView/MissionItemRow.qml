import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// One row of the mission item list
Rectangle {
    id: root

    required property var object    ///< VisualMissionItem

    signal clicked

    readonly property bool _ready:      object.readyForSaveState === VisualMissionItem.ReadyForSave
    readonly property bool _current:    object.isCurrentItem
    readonly property real _badgeSize:  ScreenTools.defaultFontPixelHeight * 1.5

    objectName:     "planPanel_missionRow_" + object.sequenceNumber
    implicitHeight: Math.max(ScreenTools.defaultFontPixelHeight * 2.2, ScreenTools.isMobile ? ScreenTools.minTouchPixels : 0)
    radius:         ScreenTools.defaultBorderRadius
    color:          _current ? qgcPal.accentSubtle : "transparent"
    border.width:   _current ? 1 : 0
    border.color:   qgcPal.accent

    QGCPalette { id: qgcPal }

    RowLayout {
        anchors.fill:       parent
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth
        anchors.rightMargin: ScreenTools.defaultFontPixelWidth
        spacing:            ScreenTools.defaultFontPixelWidth

        Rectangle {
            objectName:             "planPanel_missionRowIndex_" + root.object.sequenceNumber
            Layout.preferredWidth:  root._badgeSize
            Layout.preferredHeight: root._badgeSize
            radius:                 width / 2
            color:                  root._current ? qgcPal.accent : "transparent"
            border.width:           2
            border.color:           root._ready ? (root._current ? qgcPal.accent : qgcPal.textPrimary) : qgcPal.warningText

            property string text: root._ready ? (root.object.homePosition ? "H" : String(root.object.sequenceNumber)) : "?"

            QGCLabel {
                anchors.centerIn:   parent
                text:               parent.text
                font.pointSize:     ScreenTools.smallFontPointSize
                font.bold:          true
                color:              root._current ? qgcPal.accentText : (root._ready ? qgcPal.textPrimary : qgcPal.warningText)
            }
        }

        QGCLabel {
            Layout.fillWidth:   true
            text:               root.object.commandName
            elide:              Text.ElideRight
        }

        QGCLabel {
            objectName:         "planPanel_missionRowMeta"
            text:               root._meta()
            color:              root._ready ? qgcPal.textSecondary : qgcPal.warningText
            font.pointSize:     ScreenTools.smallFontPointSize
            visible:            text !== ""
        }
    }

    function _meta() {
        if (!_ready) {
            return qsTr("Incomplete")
        }
        if (object.isSimpleItem && object.specifiesAltitude) {
            return object.altitude.valueString + " " + object.altitude.units
        }
        return ""
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      root.clicked()
    }
}
