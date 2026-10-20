import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

GridLayout {
    property alias label:                   label.text
    property alias fact:                    _comboBox.fact
    property alias indexModel:              _comboBox.indexModel
    property var   comboBox:                _comboBox
    property real  comboBoxPreferredWidth:  -1
    property bool  vertical:                false   ///< true: label above the combo box

    columns:        vertical ? 1 : 2
    columnSpacing:  ScreenTools.defaultFontPixelWidth * 2
    rowSpacing:     0

    signal activated(int index)

    QGCLabel {
        id:                 label
        Layout.fillWidth:   !vertical
        Layout.minimumWidth: implicitWidth
    }

    FactComboBox {
        id:                     _comboBox
        Layout.preferredWidth:  comboBoxPreferredWidth
        sizeToContents:         true

        onActivated: (index) => { parent.activated(index) }
    }
}
