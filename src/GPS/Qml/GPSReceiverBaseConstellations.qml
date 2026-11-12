pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// The constellations whose observations a base sends in its corrections, for the receivers that can leave some of
/// them out. GPS observations are always sent.
GPSFactField {
    id: root

    /// The constellations the selected receiver can leave out, as RTKSettings.CorrectionConstellation bits. The
    /// others have no box: the receiver sends no observations of them, or always does.
    required property int selectable

    objectName: "rtkCorrectionConstellations"
    note: fact.shortDescription

    /// The box of one constellation, an RTKSettings.CorrectionConstellation bit of the setting.
    component ConstellationBox: QGCCheckBox {
        required property Fact selection
        required property int selectable
        required property int constellation

        visible: (selectable & constellation) !== 0
        checked: (selection.rawValue & constellation) !== 0
        focusPolicy: Qt.StrongFocus
        onClicked: selection.rawValue = checked ? selection.rawValue | constellation
                                                : selection.rawValue & ~constellation
    }

    Flow {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        spacing: ScreenTools.defaultFontPixelWidth * 2

        QGCCheckBox {
            objectName: "rtkCorrectionsGps"
            text: qsTr("GPS")
            checked: true
            enabled: false
        }
        ConstellationBox {
            objectName: "rtkCorrectionsGlonass"
            text: qsTr("GLONASS")
            selection: root.fact
            selectable: root.selectable
            constellation: RTKSettings.Glonass
        }
        ConstellationBox {
            objectName: "rtkCorrectionsGalileo"
            text: qsTr("Galileo")
            selection: root.fact
            selectable: root.selectable
            constellation: RTKSettings.Galileo
        }
        ConstellationBox {
            objectName: "rtkCorrectionsBeiDou"
            text: qsTr("BeiDou")
            selection: root.fact
            selectable: root.selectable
            constellation: RTKSettings.BeiDou
        }
        ConstellationBox {
            objectName: "rtkCorrectionsQzss"
            text: qsTr("QZSS")
            selection: root.fact
            selectable: root.selectable
            constellation: RTKSettings.Qzss
        }
    }
}
