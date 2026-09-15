import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    property alias label:                   label.text
    property alias model:                   _comboBox.model
    property alias textRole:                _comboBox.textRole
    property alias valueRole:               _comboBox.valueRole
    property alias currentIndex:            _comboBox.currentIndex
    property alias currentText:             _comboBox.currentText
    property alias currentValue:            _comboBox.currentValue
    property alias alternateText:           _comboBox.alternateText
    property var   comboBox:                _comboBox
    property real  comboBoxPreferredWidth:  -1

    spacing: ScreenTools.defaultFontPixelWidth

    signal activated(int index)

    QGCLabel {
        id:                  label
        Layout.fillWidth:    true
        Layout.minimumWidth: implicitWidth
    }

    QGCComboBox {
        id:                     _comboBox
        Layout.preferredWidth:  comboBoxPreferredWidth
        sizeToContents:         true
        onActivated: (index) => { parent.activated(index) }
    }
}
