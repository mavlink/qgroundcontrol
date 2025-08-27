Summary for Claude Code: AM32 ESC Configuration Integration for QGroundControl
Project Overview
Adding AM32 ESC configuration capabilities to QGroundControl (QGC) to allow users to read/write AM32 ESC EEPROM settings via DShot programming mode through the flight controller, eliminating the need for separate configuration tools.
Key Files to Examine
MAVLink Definitions

You can use "find -iname <filename>" to find files.

development.xml (buried in the build directory) - Contains AM32_EEPROM (ID: 292) and AM32_FW_UPDATE_PROGRESS (ID: 293) message definitions

C++ Core Implementation

AM32EepromFactGroup.h - Fact group defining all AM32 EEPROM settings as QGC facts
AM32EepromFactGroup.cpp - Implementation with EEPROM parsing, packing, and change tracking
EscStatusFactGroupListModel.h - Extended to include AM32EepromFactGroup
EscStatusFactGroupListModel.cpp - Handles AM32_EEPROM MAVLink messages
AM32EepromFact.json - Metadata for AM32 facts (min/max, units, enums)

QML UI Components

AM32SettingsComponent.qml - Main UI component for AM32 settings
ActuatorComponent.qml - Currently integrates AM32 component (needs refactoring)

Build System

Check CMakeLists.txt or qmake files for adding new QML resources and C++ sources

Current Status

Basic UI loads but has layout issues when integrated into ActuatorComponent
Need to create a separate "ESC" page in the vehicle setup menu
All facts are properly connected with change tracking
MAVLink message handling is implemented

Next Steps

Create a new ESCComponent (similar to ActuatorComponent, PowerComponent, etc.)
Register it as a separate vehicle setup page
Move AM32SettingsComponent to be the main content of this new page
Fix layout issues with proper spacing and sizing

Architecture Pattern to Follow
Look at these examples for creating a new setup page:

PowerComponent.h/cpp - Component registration
PowerComponent.qml - Page structure
PX4AutoPilotPlugin.cpp - How components are added to the setup menu

Key Integration Points

Vehicle class needs to handle AM32_EEPROM messages
ESC status model already extended with AM32EepromFactGroup
Facts use QGC's fact system with proper metadata
Change tracking implemented for efficient writes (only modified bytes)

Technical Details

AM32 EEPROM is 48 bytes (extensible to 192)
Uses DShot programming mode for read/write
Write mask allows selective byte updates
Supports multi-ESC selection and configuration
