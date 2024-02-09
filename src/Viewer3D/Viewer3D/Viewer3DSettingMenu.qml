import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.Controls

import QGroundControl.Viewer3D

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

Flickable {

    signal menuClosed(bool accept)

    property var viewer3DManager: null
    property int leftMarginSpace: ScreenTools.defaultFontPixelWidth

    id: window_body
    clip: true
    visible: true
    width: Screen.width * 0.25
    height: Screen.height * 0.2
    contentWidth: width;
    contentHeight: main_column.height
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}

    Column {
        id: main_column
        anchors{
            right: parent.right
            left: parent.left
            margins:    ScreenTools.defaultFontPixelWidth
        }
        spacing:            ScreenTools.defaultFontPixelHeight * 0.5

        RowLayout{
            anchors{
                right: parent.right
                left: parent.left
            }
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
                readOnly: true

                text: (viewer3DManager)?(viewer3DManager.viewer3DSetting.osmFilePath.rawValue):("nan")
            }
        }
        RowLayout{
            anchors{
                right: parent.right
                left: parent.left
            }
            QGCButton {
                id: map_file_btn
                Layout.alignment: Qt.AlignRight

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
            anchors{
                left: parent.left
            }
            columns:        2
            columnSpacing:  ScreenTools.defaultFontPixelHeight
            rowSpacing:     ScreenTools.defaultFontPixelWidth

            QGCLabel {
                wrapMode:           Text.WordWrap
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

    onMenuClosed: function (accept){
        if(accept === true){
            viewer3DManager.qmlBackend.osmFilePath = map_file_text_feild.text
            viewer3DManager.qmlBackend.altitudeBias = parseFloat(height_bias_textfeild.text)

            viewer3DManager.viewer3DSetting.osmFilePath.rawValue = map_file_text_feild.text
            viewer3DManager.viewer3DSetting.buildingLevelHeight.rawValue = parseFloat(bld_level_height_textfeild.text)
            viewer3DManager.viewer3DSetting.altitudeBias.rawValue = parseFloat(height_bias_textfeild.text)

            viewer3DManager.osmParser.buildingLevelHeight = parseFloat(bld_level_height_textfeild.text)
        }
    }
}
