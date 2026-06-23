import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SettingsGroupLayout {
    Layout.fillWidth:       true
    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 35
    heading:                qsTr("MAVLink Actions")
    headingDescription:     qsTr("Action JSON files should be created in the '%1' folder.").arg(QGroundControl.settingsManager.appSettings.mavlinkActionsSavePath)

    property var _mavlinkActionsSettings: QGroundControl.settingsManager.mavlinkActionsSettings

    function mavlinkActionList() {
        var fileModel = QGCFileDialogController.getFiles(QGroundControl.settingsManager.appSettings.mavlinkActionsSavePath, "*.json")
        fileModel.unshift(qsTr("<None>"))
        return fileModel
    }

    LabelledComboBox {
        Layout.fillWidth:   true
        label:              qsTr("Fly View Actions")
        model:              mavlinkActionList()
        onActivated:        (index) => index == 0 ? _mavlinkActionsSettings.flyViewActionsFile.rawValue = "" : _mavlinkActionsSettings.flyViewActionsFile.rawValue = comboBox.currentText
        enabled:            model.length > 1

        Component.onCompleted: {
            var index = comboBox.find(_mavlinkActionsSettings.flyViewActionsFile.valueString)
            comboBox.currentIndex = index == -1 ? 0 : index
        }
    }

    LabelledComboBox {
        Layout.fillWidth:   true
        label:              qsTr("Joystick Actions")
        model:              mavlinkActionList()
        onActivated:        (index) => index == 0 ? _mavlinkActionsSettings.joystickActionsFile.rawValue = "" : _mavlinkActionsSettings.joystickActionsFile.rawValue = comboBox.currentText
        enabled:            model.length > 1

        Component.onCompleted: {
            var index = comboBox.find(_mavlinkActionsSettings.joystickActionsFile.valueString)
            comboBox.currentIndex = index == -1 ? 0 : index
        }
    }
}
