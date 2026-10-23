import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Inspector header: back and step buttons when drilled in, delete, and the title of the inspected element
ColumnLayout {
    id: root

    property string namePrefix                  ///< Prefix of the objectNames of the header controls
    property string title
    property string subtitle
    property bool   drillIn:            false   ///< true: show the back and previous/next buttons
    property bool   canStepPrevious:    false
    property bool   canStepNext:        false
    property bool   deletable:          true

    signal backRequested
    signal previousRequested
    signal nextRequested
    signal deleteRequested

    spacing: ScreenTools.defaultFontPixelHeight * 0.4

    QGCPalette { id: qgcPal }

    RowLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelWidth / 2

        PlanPanelIconButton {
            objectName:     root.namePrefix + "_back"
            accessibleName: qsTr("Back")
            iconSource:     "/InstrumentValueIcons/cheveron-right.svg"
            iconRotation:   180
            visible:        root.drillIn
            onClicked:      root.backRequested()
        }

        QGCLabel {
            Layout.fillWidth:   true
            text:               root.subtitle
            color:              qgcPal.textSecondary
        }

        PlanPanelIconButton {
            objectName:     root.namePrefix + "_previous"
            accessibleName: qsTr("Previous")
            iconSource:     "/InstrumentValueIcons/cheveron-right.svg"
            iconRotation:   -90
            visible:        root.drillIn
            enabled:        root.canStepPrevious
            onClicked:      root.previousRequested()
        }

        PlanPanelIconButton {
            objectName:     root.namePrefix + "_next"
            accessibleName: qsTr("Next")
            iconSource:     "/InstrumentValueIcons/cheveron-right.svg"
            iconRotation:   90
            visible:        root.drillIn
            enabled:        root.canStepNext
            onClicked:      root.nextRequested()
        }

        PlanPanelIconButton {
            objectName:     root.namePrefix + "_delete"
            accessibleName: qsTr("Delete")
            iconSource:     "/res/TrashDelete.svg"
            danger:         true
            visible:        root.deletable
            onClicked:      root.deleteRequested()
        }
    }

    QGCLabel {
        objectName:             root.namePrefix + "_title"
        Layout.fillWidth:       true
        text:                   root.title
        font.bold:              true
        font.capitalization:    Font.AllUppercase
        font.letterSpacing:     ScreenTools.defaultFontPixelWidth * 0.1
        elide:                  Text.ElideRight
    }
}
