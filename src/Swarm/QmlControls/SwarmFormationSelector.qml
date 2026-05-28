import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import Swarm

/// @brief Formation selector component
ComboBox {
    id: root

    model: [
        { text: "None", value: SwarmFormation.None },
        { text: "Line", value: SwarmFormation.Line },
        { text: "V Formation", value: SwarmFormation.VFormation },
        { text: "Grid", value: SwarmFormation.Grid },
        { text: "Circle", value: SwarmFormation.Circle },
        { text: "Custom", value: SwarmFormation.Custom }
    ]

    textRole: "text"
    valueRole: "value"

    currentIndex: model.findIndex(f => f.value === SwarmManager.currentFormation)

    onActivated: function(index) {
        SwarmManager.setCurrentFormation(model[index].value)
    }

    // Visual customization
    contentItem: RowLayout {
        spacing: 4

        Label {
            text: formationIcon
            font {
                pixelSize: ScreenTools.defaultFontPixelHeight * 0.9
            }
        }

        Label {
            text: root.displayText
            font {
                pixelSize: ScreenTools.defaultFontPixelHeight * 0.8
            }
            color: qgcPal.windowText
        }
    }

    readonly property string formationIcon: {
        switch (SwarmManager.currentFormation) {
            case SwarmFormation.Line: return "═"
            case SwarmFormation.VFormation: return "ⅴ"
            case SwarmFormation.Grid: return "▦"
            case SwarmFormation.Circle: return "○"
            case SwarmFormation.Custom: return "✎"
            default: return "○"
        }
    }
}