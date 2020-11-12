/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Layouts  1.12

import QGroundControl                   1.0
import QGroundControl.FactSystem        1.0
import QGroundControl.FactControls      1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.SettingsManager   1.0
import QGroundControl.Controls          1.0

FirstRunPrompt {
    title:      qsTr("Vehicle Information")
    promptId:   QGroundControl.corePlugin.offlineVehicleFirstRunPromptId

    property real   _margins:               ScreenTools.defaultFontPixelWidth
    property var    _appSettings:           QGroundControl.settingsManager.appSettings
    property bool   _multipleFirmware:      !QGroundControl.singleFirmwareSupport
    property bool   _multipleVehicleTypes:  !QGroundControl.singleVehicleSupport
    property real   _fieldWidth:            ScreenTools.defaultFontPixelWidth * 16

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight

        QGCLabel {
            id:                     unitsSectionLabel
            Layout.preferredWidth:  valueRect.width
            text:                   qsTr("Specify information about the vehicle you plan to fly. If you are unsure of the correct values leave them as is.")
            wrapMode:               Text.WordWrap
        }

        Rectangle {
            id:                     valueRect
            Layout.preferredHeight: valueGrid.height + (_margins * 2)
            Layout.preferredWidth:  valueGrid.width + (_margins * 2)
            color:                  qgcPal.windowShade
            Layout.fillWidth:       true

            GridLayout {
                id:                 valueGrid
                anchors.margins:    _margins
                anchors.top:        parent.top
                anchors.left:       parent.left
                columns:            2

                QGCLabel {
                    Layout.fillWidth:   true
                    text:               qsTr("Firmware")
                    visible:            _multipleFirmware
                }
                FactComboBox {
                    Layout.preferredWidth:  _fieldWidth
                    fact:                   QGroundControl.settingsManager.appSettings.offlineEditingFirmwareClass
                    indexModel:             false
                    visible:                _multipleFirmware
                }

                QGCLabel {
                    Layout.fillWidth:   true
                    text:               qsTr("Vehicle")
                    visible:            _multipleVehicleTypes
                }
                FactComboBox {
                    Layout.preferredWidth:  _fieldWidth
                    fact:                   QGroundControl.settingsManager.appSettings.offlineEditingVehicleClass
                    indexModel:             false
                    visible:                _multipleVehicleTypes
                }
            }
        }
    }
}
