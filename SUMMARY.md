# AM32 ESC Configuration Refactor Summary

## Overview
Refactored AM32 ESC configuration in QGroundControl from being embedded in ActuatorComponent to a dedicated standalone component in the vehicle setup menu.

## What Was Done

### 1. Created AM32Component as Standalone Component
- **AM32Component.h/cc**: Full VehicleComponent implementation following PowerComponent pattern
- **AM32Component.qml**: SetupPage that hosts AM32SettingsComponent
- **AM32ComponentSummary.qml**: Summary view showing ESC status
- Component appears as "AM32 ESC" in vehicle setup menu

### 2. Enhanced AM32 Fact System
- **Added missing facts** to AM32EepromFact.json:
  - Servo settings (servoLowThreshold, servoHighThreshold, servoNeutral, servoDeadband)
  - disableStickCalibration
  - absoluteVoltageCutoff
- **Updated AM32EepromFactGroup.h/cc** to handle new facts with proper EEPROM parsing and packing
- **Fixed conversion logic** based on actual AM32 source code (defaults.h, eeprom.h)

### 3. Updated AM32SettingsComponent UI
- **ESC selection behavior**: Default all selected (green borders), click to deselect
- **Border color indicators**:
  - Green: Selected ESCs
  - Red: Missing/mismatched ESCs  
  - Purple: ESCs with unsaved changes
  - No border: Deselected ESCs
- **Settings organization** matches AM32 Configurator groups (Essentials, Motor, Extended Settings, Limits, Current Control, Sinusoidal Startup, Brake, Servo Settings)
- **Multi-ESC support**: Settings changes apply to all selected ESCs when written

### 4. Integration Changes
- **PX4AutoPilotPlugin**: Registers AM32Component when AM32 EEPROM data detected
- **Removed from ActuatorComponent**: Clean separation of concerns
- **Build system**: Updated CMakeLists.txt with new component files

### 5. Component Detection Logic
- Only shows AM32 ESC component when `vehicle.escs` contains ESCs with `am32Eeprom` property
- Assumes AM32 firmware when AM32_EEPROM messages are received
- Future extensible for other ESC protocols (BlueJay, etc.)

## Current State
- AM32Component successfully registers and appears in vehicle setup menu
- All AM32 facts properly defined with correct min/max/default values
- EEPROM read/write operations work with proper value conversions
- ESC selection UI functional with color-coded status indicators

## Known Issues
- **Layout problem**: AM32SettingsComponent UI elements are "jumbled up on top of each other" - needs layout fixes
- Component sizing and spacing needs adjustment for proper display

## Files Modified/Created
### New Files:
- `src/AutoPilotPlugins/PX4/AM32Component.h/cc/qml`
- `src/AutoPilotPlugins/PX4/AM32ComponentSummary.qml`

### Modified Files:
- `src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.h/cc`
- `src/AutoPilotPlugins/PX4/AM32SettingsComponent.qml`
- `src/Vehicle/FactGroups/AM32EepromFact.json`
- `src/Vehicle/FactGroups/AM32EepromFactGroup.h/cc`
- `src/AutoPilotPlugins/PX4/ActuatorComponent.qml`
- `src/AutoPilotPlugins/PX4/CMakeLists.txt`

### Removed Files:
- `src/AutoPilotPlugins/PX4/ESCComponent.h/cc/qml`
- `src/AutoPilotPlugins/PX4/ESCComponentSummary.qml`

## Next Steps
The functional implementation is complete, but the AM32SettingsComponent layout needs fixing to properly display the UI elements with correct spacing and positioning.