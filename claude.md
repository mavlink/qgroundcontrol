Summary for Claude Code: AM32 ESC Configuration Integration for QGroundControl

Project Overview:
Adding AM32 ESC configuration capabilities to QGroundControl (QGC) to allow users to read/write AM32 ESC EEPROM settings via DShot programming mode through the flight controller, eliminating the need for separate configuration tools.


Current Task:

AM32Setting {
	// Truth from eeprom data
	Fact settingFact -- the hardware value converted into proper units (settingFact.rawValue.uint8() to convert or whatever the type is)
	uint8_t rawValue -- the raw hardware value

	// display / unsaved values
	FactMetaData::ValueType_t unsavedValue

	// mapping
	uint8_t eepromIndex
	std::function convertRawToProper
	std::function convertProperToRaw
}


relevant files:
/home/jake/code/jake/qgroundcontrol/src/AutoPilotPlugins/PX4/AM32SettingsComponent.qml
/home/jake/code/jake/qgroundcontrol/src/AutoPilotPlugins/PX4/AM32SettingSlider.qml
/home/jake/code/jake/qgroundcontrol/src/Vehicle/FactGroups/AM32EepromFactGroup.cc
/home/jake/code/jake/qgroundcontrol/src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.cc

You might want to look at the fact system files too (src/FactSystem/)

You will not test anything, I will do the testing. You will not generate a guide.
