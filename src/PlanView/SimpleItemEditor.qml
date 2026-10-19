import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// Editor for Simple mission items
Rectangle {
    required property var missionItem
    required property real availableWidth

    id: root
    width: availableWidth
    height: editorColumn.height + (_margin * 2)
    color: qgcPal.windowShadeDark
    radius: _radius


    property bool _specifiesAltitude: missionItem.specifiesAltitude
    property real _margin: ScreenTools.defaultFontPixelHeight / 2
    property real _altRectMargin: ScreenTools.defaultFontPixelWidth / 2
    property var _controllerVehicle: missionItem.masterController.controllerVehicle
    property int _globalAltFrame: missionItem.masterController.missionController.globalAltitudeFrame
    property bool _globalAltFrameIsMixed: _globalAltFrame == QGroundControl.AltitudeFrameMixed
    property real _radius: ScreenTools.defaultFontPixelWidth / 2
    property real _fieldSpacing: ScreenTools.defaultFontPixelHeight / 2

    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    Column {
        id: editorColumn
        anchors.margins: _margin
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: _margin

        // Takeoff item
        ColumnLayout {
            anchors.margins: _margin
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: _margin
            visible: missionItem.isTakeoffItem && missionItem.wizardMode // Hack special case for takeoff item

            QGCLabel {
                text: qsTr("Move '%1' %2 to the %3 location. %4")
                    .arg(_controllerVehicle.vtol ? qsTr("T") : qsTr("T"))
                    .arg(_controllerVehicle.vtol ? qsTr("Transition Direction") : qsTr("Takeoff"))
                    .arg(_controllerVehicle.vtol ? qsTr("desired") : qsTr("climbout"))
                    .arg(_controllerVehicle.vtol ? (qsTr("Ensure distance from launch to transition direction is far enough to complete transition.")) : "")
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: !initialClickLabel.visible
            }

            QGCLabel {
                text: qsTr("Ensure clear of obstacles and into the wind.")
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: !initialClickLabel.visible
            }

            QGCButton {
                text: qsTr("Done")
                Layout.fillWidth: true
                visible: !initialClickLabel.visible
                onClicked: {
                    missionItem.wizardMode = false
                }
            }

            QGCLabel {
                id: initialClickLabel
                text: missionItem.launchTakeoffAtSameLocation ?
                                        qsTr("Click in map to set planned Takeoff location.") :
                                        qsTr("Click in map to set planned Launch location.")
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: missionItem.isTakeoffItem && !missionItem.launchCoordinate.isValid
            }
        }

        QGCGroupBox {
            width: parent.width
            contentSpacing: _fieldSpacing
            showDividers: false
            // Hidden when the item has no basic, camera or advanced values, rather than drawing an empty border
            visible: !missionItem.wizardMode && currentIndex !== -1

            QGCGroupBoxTab {
                iconSource: "/res/PlanSimpleItemBasic.svg"
                tabVisible: _specifiesAltitude || missionItem.speedSection.available || missionItem.comboboxFacts.count > 0 || missionItem.textFieldFacts.count > 0 || missionItem.nanFacts.count > 0

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: _fieldSpacing
                    visible: _specifiesAltitude

                    RowLayout {
                        Layout.fillWidth: true
                        visible: _globalAltFrameIsMixed

                        QGCLabel {
                            Layout.fillWidth: true
                            text: qsTr("Alt Frame")
                        }

                        AltFrameCombo {
                            altitudeFrame: missionItem.altitudeFrame
                            vehicle: _controllerVehicle
                            onAltitudeFrameChanged: missionItem.altitudeFrame = altitudeFrame
                        }
                    }

                    FactTextFieldSlider {
                        id: altField
                        Layout.fillWidth: true
                        label: qsTr("Altitude%1").arg(_extraLabelText())
                        fact: missionItem.altitude

                        function _extraLabelText() {
                            return qsTr(" (%1)").arg(QGroundControl.altitudeFrameExtraUnits(missionItem.altitudeFrame))
                        }
                    }

                    QGCLabel {
                        font.pointSize: ScreenTools.smallFontPointSize
                        text: qsTr("Actual AMSL alt sent: %1 %2").arg(missionItem.amslAltAboveTerrain.valueString).arg(missionItem.amslAltAboveTerrain.units)
                        visible: missionItem.altitudeFrame === QGroundControl.AltitudeFrameCalcAboveTerrain
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: _fieldSpacing

                    Repeater {
                        model: missionItem.comboboxFacts

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            QGCLabel {
                                font.pointSize: ScreenTools.smallFontPointSize
                                text: object.name
                                visible: object.name !== ""
                            }

                            FactComboBox {
                                Layout.fillWidth: true
                                indexModel: false
                                model: object.enumStrings
                                fact: object
                            }
                        }
                    }
                }

                Repeater {
                    model: missionItem.textFieldFacts

                    FactTextFieldSlider {
                        Layout.fillWidth: true
                        label: object.name
                        fact: object
                        enabled: !object.readOnly
                        warnOnUserMinMaxInvalid: false
                    }
                }

                Repeater {
                    model: missionItem.nanFacts

                    FactTextFieldSlider {
                        Layout.fillWidth: true
                        label: object.name
                        fact: object
                        showEnableCheckbox: true
                        enableCheckBoxChecked: !isNaN(object.rawValue)
                        warnOnUserMinMaxInvalid: false

                        onEnableCheckboxClicked: object.rawValue = enableCheckBoxChecked ? 0 : NaN
                    }
                }

                FactTextFieldSlider {
                    Layout.fillWidth: true
                    label: qsTr("Flight Speed")
                    fact: missionItem.speedSection.flightSpeed
                    showEnableCheckbox: true
                    enableCheckBoxChecked: missionItem.speedSection.specifyFlightSpeed
                    visible: missionItem.speedSection.available

                    onEnableCheckboxClicked: missionItem.speedSection.specifyFlightSpeed = enableCheckBoxChecked
                }
            }

            QGCGroupBoxTab {
                iconSource: "/res/PlanSimpleItemCamera.svg"
                tabVisible: missionItem.cameraSection.available

                CameraSection {
                    Layout.fillWidth: true
                    showSectionHeader: false
                    missionItem: root.missionItem

                    Component.onCompleted: checked = missionItem.cameraSection.settingsSpecified
                }
            }

            QGCGroupBoxTab {
                iconSource: "/res/PlanSimpleItemAdvanced.svg"
                tabVisible: missionItem.comboboxFactsAdvanced.count > 0 || missionItem.textFieldFactsAdvanced.count > 0 || missionItem.nanFactsAdvanced.count > 0

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: _fieldSpacing

                    Repeater {
                        model: missionItem.comboboxFactsAdvanced

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            QGCLabel {
                                font.pointSize: ScreenTools.smallFontPointSize
                                text: object.name
                                visible: object.name !== ""
                            }

                            FactComboBox {
                                Layout.fillWidth: true
                                indexModel: false
                                model: object.enumStrings
                                fact: object
                            }
                        }
                    }
                }

                Repeater {
                    model: missionItem.textFieldFactsAdvanced

                    FactTextFieldSlider {
                        Layout.fillWidth: true
                        label: object.name
                        fact: object
                        enabled: !object.readOnly
                        warnOnUserMinMaxInvalid: false
                    }
                }

                Repeater {
                    model: missionItem.nanFactsAdvanced

                    FactTextFieldSlider {
                        Layout.fillWidth: true
                        label: object.name
                        fact: object
                        showEnableCheckbox: true
                        enableCheckBoxChecked: !isNaN(object.rawValue)
                        warnOnUserMinMaxInvalid: false

                        onEnableCheckboxClicked: object.rawValue = enableCheckBoxChecked ? 0 : NaN
                    }
                }
            }
        }
    }
}
