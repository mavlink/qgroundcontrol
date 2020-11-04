import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Layouts          1.2

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

SpinBox {
    id:                 root
    implicitHeight:     ScreenTools.implicitTextFieldHeight

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    onEditingFinished: {
        if (ScreenTools.isMobile) {
            // Toss focus on mobile after Done on virtual keyboard. Prevent strange interactions.
            focus = false
        }
    }

    style: SpinBoxStyle {
        id:             sbs
        textColor:      qgcPal.textFieldText
        font.pointSize: ScreenTools.defaultFontPointSize
        font.family:    ScreenTools.normalFontFamily
        renderType:     ScreenTools.isWindows ? Text.QtRendering : sbs.renderType   // This works around font rendering problems on windows
    }
}
