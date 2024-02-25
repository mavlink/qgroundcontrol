/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FactSystem
import QGroundControl.FactControls

ColumnLayout {
    spacing: ScreenTools.defaultFontPixelHeight / 2

    FactPanelController { id: controller }

    SettingsGroupLayout {
        heading:            qsTr("Ground Control Data Link Loss")
        Layout.fillWidth:   true

        RowLayout {
            Layout.fillWidth: true
            spacing:          ScreenTools.defaultFontPixelWidth * 2

            QGCLabel {
                Layout.fillWidth:   true;
                text:               qsTr("Failsafe Action")
            }
            FactComboBox {
                id:                     failsafeActionCombo
                fact:                   controller.getParameterFact(-1, "NAV_DLL_ACT")
                indexModel:             false
            }
        }

        LabelledFactSlider {
            Layout.fillWidth:       true
            label:                  qsTr("Data Link Loss Timeout")
            fact:                   controller.getParameterFact(-1, "COM_DL_LOSS_T")
            sliderPreferredWidth:   ScreenTools.defaultFontPixelWidth * 20
        }
    }
}
