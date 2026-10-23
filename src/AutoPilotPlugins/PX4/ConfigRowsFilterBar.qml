import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Show-all / show-some toggle with a count summary. Extra children follow the summary.
RowLayout {
    property alias  filterLabels:   filterBar.model
    property alias  filterIndex:    filterBar.currentIndex
    property alias  summary:        summaryLabel.text

    signal filterActivated(int index)

    Layout.fillWidth:   true
    spacing:            ScreenTools.defaultFontPixelHeight * 0.5

    QGCPalette { id: qgcPal }

    QGCRadioButtonBar {
        id:             filterBar
        onActivated:    (index) => filterActivated(index)
    }

    QGCLabel {
        id:                 summaryLabel
        Layout.fillWidth:   true
        color:              qgcPal.textSecondary
    }
}
