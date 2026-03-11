import QGroundControl
import QGroundControl.AppSettings
import QGroundControl.Controls
import QGroundControl.FactControls
import QtQuick
import QtQuick.Layouts

SettingsPage {
    property Fact _viewer3DAltitudeBias: _viewer3DSettings.altitudeBias
    property Fact _viewer3DBuildingLevelHeight: _viewer3DSettings.buildingLevelHeight
    property Fact _viewer3DEnabled: _viewer3DSettings.enabled
    property Fact _viewer3DMapProvider: _viewer3DSettings.mapProvider
    property Fact _viewer3DOsmFilePath: _viewer3DSettings.osmFilePath
    property var _viewer3DSettings: QGroundControl.settingsManager.viewer3DSettings

    SettingsGroupLayout {
        Layout.fillWidth: true
        heading: qsTr("3D View")

        FactCheckBoxSlider {
            Layout.fillWidth: true
            fact: _viewer3DEnabled
            text: qsTr("Enabled")
            visible: _viewer3DEnabled.visible
        }

        LabelledFactComboBox {
            Layout.fillWidth: true
            enabled: _viewer3DEnabled.rawValue
            fact: _viewer3DMapProvider
            label: qsTr("Map Provider")
            visible: _viewer3DMapProvider.visible
        }

        ColumnLayout {
            Layout.fillWidth: true
            enabled: _viewer3DEnabled.rawValue
            spacing: ScreenTools.defaultFontPixelWidth
            visible: _viewer3DMapProvider.rawValue === 0

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    text: qsTr("3D Map File:")
                    wrapMode: Text.WordWrap
                }

                QGCTextField {
                    id: osmFileTextField

                    Layout.fillWidth: true
                    height: ScreenTools.defaultFontPixelWidth * 4.5
                    readOnly: true
                    showUnits: false
                    text: _viewer3DOsmFilePath.rawValue
                    unitsLabel: ""
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text: qsTr("Clear")

                    onClicked: {
                        _viewer3DOsmFilePath.value = "";
                    }
                }

                QGCButton {
                    text: qsTr("Select File")

                    onClicked: {
                        var filename = _viewer3DOsmFilePath.rawValue;
                        const found = filename.match(/(.*)[\/\\]/);
                        if (found) {
                            filename = found[1] || ''; // extracting the directory from the file path
                            fileDialog.folder = (filename[0] === "/") ? (filename.slice(1)) : (filename);
                        }
                        fileDialog.openForLoad();
                    }

                    QGCFileDialog {
                        id: fileDialog

                        nameFilters: [qsTr("OpenStreetMap files (*.osm)")]
                        title: qsTr("Select map file")

                        onAcceptedForLoad: file => {
                            _viewer3DOsmFilePath.value = file;
                        }
                    }
                }
            }
        }

        LabelledFactTextField {
            Layout.fillWidth: true
            enabled: _viewer3DEnabled.rawValue
            fact: _viewer3DBuildingLevelHeight
            label: qsTr("Average Building Level Height")
            visible: _viewer3DBuildingLevelHeight.visible
        }

        LabelledFactTextField {
            Layout.fillWidth: true
            enabled: _viewer3DEnabled.rawValue
            fact: _viewer3DAltitudeBias
            label: qsTr("Vehicles Altitude Bias")
            visible: _viewer3DAltitudeBias.visible
        }
    }
}
