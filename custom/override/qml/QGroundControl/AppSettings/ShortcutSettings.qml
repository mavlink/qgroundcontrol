import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SettingsPage{
    SettingsGroupLayout{
        Repeater{
            model: globalShortcuts
            delegate:LabelledLabel{
                label:description
                labelText: sequence
            }
        }
    }
}
