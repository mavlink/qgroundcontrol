import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QtQuick
import QtQuick.Layouts

SettingsGroupLayout {
    id: root

    property var positionManager: QGroundControl.qgcPositionManger
    property var settings: QGroundControl.settingsManager.gpsPositionSettings

    heading: qsTr("Ground-Station Position Source")
    visible: root.settings && root.settings.userVisible

    LabelledFactComboBox {
        Layout.fillWidth: true
        fact: root.settings ? root.settings.sourceMode : null
        indexModel: false
        label: qsTr("Position source")
        objectName: "gpsPositionSourceMode"
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        objectName: "gpsPositionSourceName"
        text: root.positionManager ? qsTr("Current source: %1").arg(root.positionManager.selectedSourceName) : qsTr("No position source")
        wrapMode: Text.WordWrap
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        objectName: "gpsPositionSourceReason"
        text: root.positionManager ? root.positionManager.selectionReason : ""
        wrapMode: Text.WordWrap
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        objectName: "gpsPositionSourceStatus"
        text: root.positionManager ? root.positionManager.sourceStatusText : qsTr("No position source")
        wrapMode: Text.WordWrap
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        text: qsTr("Automatic selection can change the source when a fix becomes unavailable. Keep all eligible receivers with the ground station.")
        visible: root.settings && root.settings.sourceMode.rawValue === 1
        wrapMode: Text.WordWrap
    }
}
