import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// One required component in the setup checklist on the Summary page. Opens the component's setup page when clicked.
Rectangle {
    id: control

    property string text
    property bool   done:       false
    property bool   next:       false
    property bool   clickable:  true

    signal clicked()

    Layout.fillWidth:   true
    implicitHeight:     rowLayout.implicitHeight + ScreenTools.defaultFontPixelWidth * 2
    radius:             ScreenTools.defaultBorderRadius
    color:              next ? qgcPal.accentSubtle : qgcPal.windowCard
    border.color:       done ? qgcPal.colorGreen : (next || (clickable && mouseArea.containsMouse)) ? qgcPal.accent : qgcPal.borderSubtle

    QGCPalette { id: qgcPal }

    MouseArea {
        id:             mouseArea
        anchors.fill:   parent
        hoverEnabled:   true
        cursorShape:    control.clickable ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: {
            if (control.clickable) {
                control.clicked()
            }
        }
    }

    RowLayout {
        id:                 rowLayout
        anchors.fill:       parent
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        spacing:            ScreenTools.defaultFontPixelWidth

        VehicleSummaryLabel {
            Layout.fillWidth:   true
            text:               control.text
            font.bold:          true
            elide:              Text.ElideRight
        }

        VehicleSummaryLabel {
            text:           control.done ? qsTr("Done") : control.next ? qsTr("Next") : qsTr("To do")
            font.pointSize: ScreenTools.smallFontPointSize
            font.bold:      control.done || control.next
            color:          control.done ? qgcPal.successText : control.next ? qgcPal.accent : qgcPal.textSecondary
        }

        Rectangle {
            implicitWidth:  ScreenTools.defaultFontPixelHeight * 1.1
            implicitHeight: implicitWidth
            radius:         implicitWidth / 2
            color:          control.done ? qgcPal.colorGreen : "transparent"
            border.color:   control.done ? qgcPal.colorGreen : control.next ? qgcPal.accent : qgcPal.borderSubtle
            border.width:   control.next ? 2 : 1

            VehicleSummaryLabel {
                anchors.centerIn:   parent
                text:               "✓"
                font.pointSize:     ScreenTools.smallFontPointSize
                font.bold:          true
                color:              qgcPal.window
                visible:            control.done
            }
        }
    }
}
