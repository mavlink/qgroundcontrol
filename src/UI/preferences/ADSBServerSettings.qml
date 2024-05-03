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

import QGroundControl
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls

SettingsPage {
    property var    _settingsManager:           QGroundControl.settingsManager
    property var     _adsbSettings:             _settingsManager.adsbVehicleManagerSettings
    property Fact   _adsbServerConnectEnabled:  _adsbSettings.adsbServerConnectEnabled

    SettingsGroupLayout {
        Layout.fillWidth:   true
        visible:            QGroundControl.settingsManager.adsbVehicleManagerSettings.visible

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               fact.shortDescription
            fact:               _adsbServerConnectEnabled
            visible:            fact.visible
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        visible:             _adsbSettings.adsbServerHostAddress.visible || _adsbSettings.adsbServerPort.visible
        enabled:             _adsbServerConnectEnabled.rawValue

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              fact.shortDescription
            fact:               _adsbSettings.adsbServerHostAddress
            visible:            fact.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              fact.shortDescription
            fact:               _adsbSettings.adsbServerPort
            visible:            fact.visible
        }
    }
}
