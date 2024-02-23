import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.FactControls

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

QGCPopupDialog{
    property var viewer3DManager:                       null
    property var _viewer3DSetting:                   QGroundControl.settingsManager.viewer3DSettings

    title:      qsTr("3D view settings")
    buttons:    Dialog.Ok | Dialog.Cancel

    ColumnLayout {
        property int leftMarginSpace: ScreenTools.defaultFontPixelWidth

        id: window_body
        clip: true
        visible: true
        spacing: ScreenTools.defaultFontPixelHeight / 2
        RowLayout{
            Layout.fillWidth:   true
            QGCLabel {
                wrapMode:           Text.WordWrap
                visible:            true
                text: qsTr("3D Map File:")
            }

            QGCTextField {
                id:                 map_file_text_feild
                height:             ScreenTools.defaultFontPixelWidth * 4.5
                unitsLabel:         ""
                showUnits:          false
                visible:            true
                Layout.fillWidth:   true
                implicitWidth: ScreenTools.defaultFontPixelWidth * 50


                readOnly: true

                text: _viewer3DSetting.osmFilePath.rawValue
            }
        }
        RowLayout{
            Layout.alignment: Qt.AlignRight

            QGCButton {
                id: map_file_btn

                visible:    true
                text:       qsTr("Select File")

                onClicked: {
                    fileDialog.openForLoad()
                }

                QGCFileDialog {
                    id:             fileDialog

                    nameFilters:    [qsTr("OpenStreetMap files (*.osm)")]
                    title:          qsTr("Select map file")
                    onAcceptedForLoad: (file) => {
                                           map_file_text_feild.text = file
                                       }
                }
            }
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Average Building Level Height")
            fact:               _viewer3DSetting.buildingLevelHeight
            visible:            fact.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              qsTr("Vehicles Altitude Bias")
            fact:               _viewer3DSetting.altitudeBias
            visible:            fact.visible
        }
    }

    onAccepted: {
        _viewer3DSetting.osmFilePath.value = map_file_text_feild.text
    }
}
