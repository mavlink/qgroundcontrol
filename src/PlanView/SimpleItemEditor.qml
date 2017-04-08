import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0

// Editor for Simple mission items
Rectangle {
    id:                 valuesRect
    width:              availableWidth
    height:             deferedload.status == Loader.Ready ? (visible ? deferedload.item.height : 0) : 0
    color:              qgcPal.windowShadeDark
    visible:            missionItem.isCurrentItem
    radius:             _radius

    Loader {
        id:              deferedload
        active:          valuesRect.visible
        asynchronous:    true
        anchors.margins: _margin
        anchors.left:    valuesRect.left
        anchors.right:   valuesRect.right
        anchors.top:     valuesRect.top

        sourceComponent: Component {
            Item {
                id:                 valuesItem
                height:             valuesColumn.height + (_margin * 2)

                Column {
                    id:             valuesColumn
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    anchors.top:    parent.top
                    spacing:        _margin

                    QGCLabel {
                        width:          parent.width
                        wrapMode:       Text.WordWrap
                        font.pointSize: ScreenTools.smallFontPointSize
                        text:           missionItem.rawEdit ?
                                            qsTr("Provides advanced access to all commands/parameters. Be very careful!") :
                                            missionItem.commandDescription
                    }

                    GridLayout {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        columns:        2

                        Repeater {
                            model: missionItem.comboboxFacts

                            QGCLabel {
                                text:           object.name
                                visible:        object.name != ""
                                Layout.column:  0
                                Layout.row:     index
                            }
                        }

                        Repeater {
                            model: missionItem.comboboxFacts

                            FactComboBox {
                                indexModel:         false
                                model:              object.enumStrings
                                fact:               object
                                Layout.column:      1
                                Layout.row:         index
                                Layout.fillWidth:   true
                            }
                        }
                    }

                    GridLayout {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        columns:        2

                        Repeater {
                            model: missionItem.textFieldFacts

                            QGCLabel {
                                text:           object.name
                                Layout.column:  0
                                Layout.row:     index
                            }
                        }

                        Repeater {
                            model: missionItem.textFieldFacts

                            FactTextField {
                                showUnits:          true
                                fact:               object
                                Layout.column:      1
                                Layout.row:         index
                                Layout.fillWidth:   true
                            }
                        }
                    }

                    GridLayout {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        columns:        2

                        Repeater {
                            model: missionItem.nanFacts

                            QGCCheckBox {
                                text:           object.name
                                Layout.column:  0
                                Layout.row:     index
                                checked:        isNaN(object.rawValue)
                                onClicked:      object.rawValue = checked ? NaN : 0
                            }
                        }

                        Repeater {
                            model: missionItem.nanFacts

                            FactTextField {
                                showUnits:          true
                                fact:               object
                                Layout.column:      1
                                Layout.row:         index
                                Layout.fillWidth:   true
                                enabled:            !isNaN(object.rawValue)
                            }
                        }
                    }

                    Repeater {
                        model: missionItem.checkboxFacts

                        FactCheckBox {
                            text:   object.name
                            fact:   object
                        }
                    }

                    CameraSection {
                        checked:    missionItem.cameraSection.settingsSpecified
                        visible:    missionItem.cameraSection.available
                    }
                } // Column
            } // Item
        } // Component
    } // Loader
} // Rectangle
