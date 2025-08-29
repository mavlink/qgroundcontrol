Summary for Claude Code: AM32 ESC Configuration Integration for QGroundControl

Project Overview:
Adding AM32 ESC configuration capabilities to QGroundControl (QGC) to allow users to read/write AM32 ESC EEPROM settings via DShot programming mode through the flight controller, eliminating the need for separate configuration tools.

Current Task:

Fix the fixme in AM32SettingsComponent.qml. You'll have to add to AM32EepromFactGroup. I suggest you implement a setting name/fact/byteIndex mapping with a nice helper function and maybe even some qml if you need it. It should track the reported value and the user selected value. When those are the same (the initialization state of the whole page) we don't mark any changes as pending. I think we could really simpify things in AM32EepromFactGroup by doing that. Then it will make our qml logic easier to follow as well.

relevant files:
/home/jake/code/jake/qgroundcontrol/src/AutoPilotPlugins/PX4/AM32SettingsComponent.qml
/home/jake/code/jake/qgroundcontrol/src/AutoPilotPlugins/PX4/AM32SettingSlider.qml
/home/jake/code/jake/qgroundcontrol/src/Vehicle/FactGroups/AM32EepromFactGroup.cc
/home/jake/code/jake/qgroundcontrol/src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.cc

You might want to look at the fact system files too (src/FactSystem/)
