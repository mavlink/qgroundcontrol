/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.ScreenTools
import QGroundControl.Controllers

QGCPopupDialog {
    id:         root
    title:      qsTr("Edit Position")
    buttons:    Dialog.Close

    property alias coordinate:                  controller.coordinate
    property bool  showSetPositionFromVehicle:  true

    property real   _margin:        ScreenTools.defaultFontPixelWidth / 2
    property real   _fieldWidth:    ScreenTools.defaultFontPixelWidth * 10.5

    EditPositionDialogController {
        id: controller

        Component.onCompleted: initValues()
    }

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight / 2

        SettingsGroupLayout {
            heading:   qsTr("Geographic Position")

            LabelledFactTextField {
                label:              qsTr("Latitude")
                fact:               controller.latitude
                Layout.fillWidth:   true
            }

            LabelledFactTextField {
                label:              qsTr("Longitude")
                fact:               controller.longitude
                Layout.fillWidth:   true
            }

            LabelledButton {
                label:               qsTr("Set position")
                buttonText:          qsTr("Move")
                onClicked: {
                    controller.setFromGeo()
                    root.close()
                }
            }
        }

        SettingsGroupLayout {
            heading:   qsTr("UTM Position")

            LabelledFactTextField {
                label:              qsTr("Zone")
                fact:               controller.zone
                Layout.fillWidth:   true
            }

            LabelledFactComboBox {
                label:              qsTr("Hemisphere")
                fact:               controller.hemisphere
                indexModel:         false
                Layout.fillWidth:   true
            }

            LabelledFactTextField {
                label:              qsTr("Easting")
                fact:               controller.easting
                Layout.fillWidth:   true
            }

            LabelledFactTextField {
                label:              qsTr("Northing")
                fact:               controller.northing
                Layout.fillWidth:   true
            }

            LabelledButton {
                label:               qsTr("Set position")
                buttonText:          qsTr("Move")
                onClicked: {
                    controller.setFromUTM()
                    root.close()
                }
            }
        }

        SettingsGroupLayout {
            heading:  qsTr("MGRS Position")

            LabelledFactTextField {
                label:              qsTr("MGRS")
                fact:               controller.mgrs
                Layout.fillWidth:   true
            }

            LabelledButton {
                label:               qsTr("Set position")
                buttonText:          qsTr("Move")
                onClicked: {
                    controller.setFromMGRS()
                    root.close()
                }
            }
        }

        SettingsGroupLayout {
            heading:    qsTr("Set From Vehicle Position")
            visible:    showSetPositionFromVehicle

            LabelledButton {
                label:               qsTr("Set position")
                buttonText:          qsTr("Move")
                onClicked: {
                    controller.setFromVehicle()
                    root.close()
                }
            }
        }
    }
}
