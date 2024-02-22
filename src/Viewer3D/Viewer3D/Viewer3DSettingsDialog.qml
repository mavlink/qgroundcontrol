import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.Controls


///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

QGCPopupDialog{
    property var viewer3DManager: null

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

                text: (viewer3DManager)?(viewer3DManager.viewer3DSetting.osmFilePath.rawValue):("nan")
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

        GridLayout{
            columns:        2
            columnSpacing:  ScreenTools.defaultFontPixelHeight
            rowSpacing:     ScreenTools.defaultFontPixelWidth
            Layout.fillWidth:   true

            QGCLabel {
                wrapMode:           Text.WordWrap
                Layout.fillWidth:   true
                text: qsTr("Average Building Level Height:")
            }

            QGCTextField{
                id:                 bld_level_height_textfeild
                width:              ScreenTools.defaultFontPixelWidth * 15
                unitsLabel:         "m"
                showUnits:          true
                numericValuesOnly:  true
                visible:            true

                text: (viewer3DManager)?(Number(viewer3DManager.viewer3DSetting.buildingLevelHeight.rawValue)):("nan")

                validator: RegularExpressionValidator{
                    regularExpression: /(-?\d{1,10})([.]\d{1,6})?$/
                }

                onAccepted:
                {
                    focus = false
                }
            }

            QGCLabel {
                wrapMode:           Text.WordWrap
                visible:            true
                Layout.fillWidth:   true
                text: qsTr("Vehicles Altitude Bias:")
            }

            QGCTextField {
                id:                 height_bias_textfeild
                width:              ScreenTools.defaultFontPixelWidth * 15
                unitsLabel:         "m"
                showUnits:          true
                numericValuesOnly:  true
                visible:            true

                text: (viewer3DManager)?(Number(viewer3DManager.viewer3DSetting.altitudeBias.rawValue)):("nan")

                validator: RegularExpressionValidator{
                    regularExpression: /(-?\d{1,10})([.]\d{1,6})?$/
                }

                onAccepted:
                {
                    focus = false
                }
            }
        }
    }

    onAccepted: {
        viewer3DManager.qmlBackend.osmFilePath = map_file_text_feild.text
        viewer3DManager.qmlBackend.altitudeBias = parseFloat(height_bias_textfeild.text)

        viewer3DManager.viewer3DSetting.osmFilePath.rawValue = map_file_text_feild.text
        viewer3DManager.viewer3DSetting.buildingLevelHeight.rawValue = parseFloat(bld_level_height_textfeild.text)
        viewer3DManager.viewer3DSetting.altitudeBias.rawValue = parseFloat(height_bias_textfeild.text)

        viewer3DManager.osmParser.buildingLevelHeight = parseFloat(bld_level_height_textfeild.text)
    }
}
