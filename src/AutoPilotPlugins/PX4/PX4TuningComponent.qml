import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    id:         root
    spacing:    ScreenTools.defaultFontPixelWidth / 4

    property var model

    property real _availableHeight: availableHeight
    property real _availableWidth:  availableWidth

    FactPanelController {
        id:         controller
    }

    QGCGroupBox {
        id: tuningGroup
        showDividers: false
        contentSpacing: 0

        Repeater {
            model: root.model

            QGCGroupBoxTab {
                title: buttonText
                buttonObjectName: "pidTuning_tab_" + buttonText.replace(/ /g, "")
            }
        }

        // A single loader tears down the previous tuning page before creating the next one, so the outgoing page
        // cannot disable the tuning telemetry the incoming page just enabled
        Loader {
            id:     loader
            source: root.model.get(tuningGroup.currentIndex).tuningPage

            property bool useAutoTuning:    true
            property real availableWidth:   _availableWidth - tuningGroup.contentSideInset * 2
            property real availableHeight:  _availableHeight - tuningGroup.y - tuningGroup.contentTopInset - loader.y - tuningGroup.contentSideInset
        }
    }
}
