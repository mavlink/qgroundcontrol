/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.MultiVehicleManager
import QGroundControl.Palette

Item {
    id: root

    property var    _settingsManager:               QGroundControl.settingsManager
    property var    _appSettings:                   _settingsManager.appSettings
    property var    _mapsSettings:                  _settingsManager.mapsSettings
    property bool   _currentlyImportOrExporting:    MapEngineManager.importAction === MapEngineManager.ActionExporting || MapEngineManager.importAction === MapEngineManager.ActionImporting
    property real   _largeTextFieldWidth:           ScreenTools.defaultFontPixelWidth * 30

    property Fact   _mapProviderFact:   _settingsManager.flightMapSettings.mapProvider
    property Fact   _mapTypeFact:       _settingsManager.flightMapSettings.mapType
    property Fact   _mapboxFact:        _settingsManager ? _settingsManager.appSettings.mapboxToken : null
    property Fact   _mapboxAccountFact: _settingsManager ? _settingsManager.appSettings.mapboxAccount : null
    property Fact   _mapboxStyleFact:   _settingsManager ? _settingsManager.appSettings.mapboxStyle : null
    property Fact   _esriFact:          _settingsManager ? _settingsManager.appSettings.esriToken : null
    property Fact   _customURLFact:     _settingsManager ? _settingsManager.appSettings.customURL : null
    property Fact   _vworldFact:        _settingsManager ? _settingsManager.appSettings.vworldToken : null

    SettingsPage {
        id:           settingsPage
        anchors.fill: parent

        Component.onCompleted: {
            MapEngineManager.loadTileSets()
        }

        Connections {
            target:                 MapEngineManager
            onErrorMessageChanged:  errorDialogComponent.createObject(mainWindow).open()
        }

        SettingsGroupLayout {
            Layout.fillWidth: true

            LabelledComboBox {
                label:      qsTr("Provider")
                model:      MapEngineManager.mapProviderList

                onActivated: (index) => {
                    _mapProviderFact.rawValue = comboBox.textAt(index)
                    _mapTypeFact.rawValue = MapEngineManager.mapTypeList(comboBox.textAt(index))[0]
                }

                Component.onCompleted: {
                    var index = comboBox.find(_mapProviderFact.rawValue)
                    if (index < 0) index = 0
                    comboBox.currentIndex = index
                }
            }

            LabelledComboBox {
                label: qsTr("Type")
                model: MapEngineManager.mapTypeList(_mapProviderFact.rawValue)

                onActivated: (index) => { _mapTypeFact.rawValue = comboBox.textAt(index) }

                Component.onCompleted: {
                    var index = comboBox.find(_mapTypeFact.rawValue)
                    if (index < 0) index = 0
                    comboBox.currentIndex = index
                }
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth:   true
            heading:            qsTr("Offline Maps")
            headingDescription: qsTr("Download map tiles for use when offline")

            Repeater {
                model: MapEngineManager.tileSets

                OfflineMapInfo {
                    tileSet:    object
                    enabled:    !object.deleting
                    onClicked:  offlineMapEditorComponent.createObject(root, { tileSet: object }).showInfo()
                }
            }

            LabelledButton {
                label:      qsTr("Add New Set")
                buttonText: qsTr("Add")
                enabled:    !_currentlyImportOrExporting
                onClicked:  offlineMapEditorComponent.createObject(root).addNewSet()
            }

            LabelledButton {
                label:      qsTr("Import Map Tiles")
                buttonText: qsTr("Import")
                visible:    QGroundControl.corePlugin.options.showOfflineMapImport
                enabled:    !_currentlyImportOrExporting
                onClicked: {
                    MapEngineManager.importAction = MapEngineManager.ActionNone
                    importDialogComponent.createObject(mainWindow).open()
                }
            }

            LabelledButton {
                label:      qsTr("Export Map Tiles")
                buttonText: qsTr("Export")
                visible:    QGroundControl.corePlugin.options.showOfflineMapExport
                enabled:    !_currentlyImportOrExporting
                onClicked:  exportDialogComponent.createObject(mainWindow).open()
            }

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth
                visible: _currentlyImportOrExporting

                QGCLabel {
                    Layout.fillWidth:   true
                    text:               MapEngineManager.importAction === MapEngineManager.ActionExporting ? qsTr("Exporting") : qsTr("Importing")
                    font.bold:          true
                }
                ProgressBar {
                    width:          ScreenTools.defaultFontPixelWidth * 25
                    from:           0
                    to:             100
                    value:          MapEngineManager.actionProgress
                }
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth:   true
            heading:            qsTr("Tokens")
            headingDescription: qsTr("Allows access to additional providers")

            LabelledFactTextField {
                textFieldPreferredWidth:    _largeTextFieldWidth
                label:                      qsTr("Mapbox")
                fact:                       _appSettings.mapboxToken
            }

            LabelledFactTextField {
                textFieldPreferredWidth:    _largeTextFieldWidth
                label:                      qsTr("Esri")
                fact:                       _appSettings.esriToken
            }

            LabelledFactTextField {
                textFieldPreferredWidth:    _largeTextFieldWidth
                label:                      qsTr("VWorld")
                fact:                       _appSettings.vworldToken
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth:   true
            heading:            qsTr("Mapbox Login")

            LabelledFactTextField {
                textFieldPreferredWidth:    _largeTextFieldWidth
                label:                      qsTr("Account")
                fact:                       _appSettings.mapboxAccount
            }

            LabelledFactTextField {
                textFieldPreferredWidth:    _largeTextFieldWidth
                label:                      qsTr("Map Style")
                fact:                       _appSettings.mapboxStyle
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth:   true
            heading:            qsTr("Custom Map URL")
            headingDescription: qsTr("URL with {x} {y} {z} or {zoom} substitutions")

            LabelledFactTextField {
                textFieldPreferredWidth:    _largeTextFieldWidth
                label:                      qsTr("Server URL")
                fact:                       _appSettings.customURL
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth:   true
            heading:            qsTr("Tile Cache")

            LabelledFactTextField {
                fact: _mapsSettings.maxCacheDiskSize
            }

            LabelledFactTextField {
                fact: _mapsSettings.maxCacheMemorySize
            }    
        }

        QGCFileDialog {
            id:             fileDialog
            folder:         _appSettings.missionSavePath
            nameFilters:    [ qsTr("Tile Sets (*.%1)").arg(defaultSuffix) ]
            defaultSuffix:  _appSettings.tilesetFileExtension

            onAcceptedForSave: (file) => {
                close()
                MapEngineManager.exportSets(file)
            }

            onAcceptedForLoad: (file) => {
                close()
                MapEngineManager.importSets(file)
            }
        }

        Component {
            id: exportDialogComponent

            QGCPopupDialog {
                title:      qsTr("Export Selected Tile Sets")
                buttons:    Dialog.Ok | Dialog.Cancel

                onAccepted: {
                    close()
                    fileDialog.title = qsTr("Export Tiles")
                    fileDialog.openForSave()
                }

                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelWidth / 2

                    Repeater {
                        model: MapEngineManager.tileSets

                        QGCCheckBox {
                            text:       object.name
                            checked:    object.selected
                            onClicked:  object.selected = checked
                        }
                    }
                }
            }
        }

        Component {
            id: importDialogComponent

            QGCPopupDialog {
                title:      qsTr("Import TileSets")
                buttons:    Dialog.Ok | Dialog.Cancel

                onAccepted: {
                    close()
                    fileDialog.title = qsTr("Import Tiles")
                    fileDialog.openForLoad()
                }

                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelWidth / 2

                    QGCRadioButton {
                        text:           qsTr("Append to existing sets")
                        checked:        !MapEngineManager.importReplace
                        onClicked:      MapEngineManager.importReplace = !checked
                    }
                    QGCRadioButton {
                        text:           qsTr("Replace existing sets")
                        checked:        MapEngineManager.importReplace
                        onClicked:      MapEngineManager.importReplace = checked
                    }
                }
            }
        }

        Component {
            id: errorDialogComponent

            QGCSimpleMessageDialog {
                title:      qsTr("Error Message")
                text:       MapEngineManager.errorMessage
                buttons:    Dialog.Close
            }
        }
    }

    Component {
        id: offlineMapEditorComponent

        OfflineMapEditor {
            id:             offlineMapEditor
            anchors.fill:   parent
        }
    }
}
