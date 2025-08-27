# AM32 ESC Configuration Refactoring Plan

## Executive Summary
Move AM32 ESC configuration from ActuatorComponent to a new dedicated ESC page in QGroundControl's vehicle setup menu, following established component patterns while maintaining AM32 Configurator's UI design.

## Phase 1: Create ESC Component Infrastructure

### 1.1 Create ESCComponent Header (ESCComponent.h)
- **Location:** `src/AutoPilotPlugins/PX4/ESCComponent.h`
- **Base:** Inherit from `VehicleComponent`
- **Key Methods to Implement:**
  - `name()` - Return "ESC"
  - `description()` - Return "ESC Configuration"
  - `iconResource()` - Return "/qmlimages/EscIndicator.svg"
  - `requiresSetup()` - Return false (no required setup)
  - `setupComplete()` - Return true (always complete)
  - `setupSource()` - Return "qrc:/qml/ESCComponent.qml"
  - `summaryQmlSource()` - Return "qrc:/qml/ESCComponentSummary.qml"

### 1.2 Create ESCComponent Implementation (ESCComponent.cc)
- **Location:** `src/AutoPilotPlugins/PX4/ESCComponent.cc`
- **Constructor:** Call parent with `AutoPilotPlugin::KnownESCVehicleComponent`
- **Note:** Need to add `KnownESCVehicleComponent` to `AutoPilotPlugin::VehicleComponentKnown` enum
- **Simple implementations for all virtual methods**

### 1.3 Create ESCComponent Main QML (ESCComponent.qml)
- **Location:** `src/AutoPilotPlugins/PX4/ESCComponent.qml`
- **Structure:**
  ```qml
  SetupPage {
      id:             escPage
      pageComponent:  escPageComponent
      
      Component {
          id: escPageComponent
          
          Item {
              // Container for AM32-specific component
              AM32SettingsComponent {
                  anchors.fill: parent
              }
          }
      }
  }
  ```

### 1.4 Create ESCComponentSummary QML
- **Location:** `src/AutoPilotPlugins/PX4/ESCComponentSummary.qml`
- **Content:** Basic summary showing ESC count and AM32 availability

## Phase 2: Register Component in PX4AutoPilotPlugin

### 2.1 Update PX4AutoPilotPlugin.h
- Add forward declaration: `class ESCComponent;`
- Add member variable: `ESCComponent* _escComponent;`

### 2.2 Update PX4AutoPilotPlugin.cc
- Add include: `#include "ESCComponent.h"`
- Initialize in constructor: `_escComponent = nullptr`
- In `vehicleComponents()`, add conditional registration:
  ```cpp
  // Only add ESC component if ESCs are available
  if (_vehicle->escs() && _vehicle->escs()->count() > 0) {
      // Check if any ESC has AM32 support
      bool hasAM32Support = false;
      for (int i = 0; i < _vehicle->escs()->count(); i++) {
          auto esc = _vehicle->escs()->get(i);
          if (esc && esc->property("am32Eeprom").isValid()) {
              hasAM32Support = true;
              break;
          }
      }
      
      if (hasAM32Support) {
          _escComponent = new ESCComponent(_vehicle, this, this);
          _escComponent->setupTriggerSignals();
          _components.append(QVariant::fromValue(static_cast<VehicleComponent*>(_escComponent)));
      }
  }
  ```

## Phase 3: Refactor AM32SettingsComponent

### 3.1 Update AM32SettingsComponent Structure
- Keep AM32SettingsComponent.qml as dedicated AM32 UI
- Update to match AM32 Configurator design:
  - ESC selection grid at top with status indicators
  - Setting groups organized like AM32 Configurator:
    - Essentials (Protocol selection)
    - Motor (timing, startup, KV, poles, PWM)
    - Extended Settings (ramp rate, min duty cycle)
    - Limits (temperature, current, voltage)
    - Current Control (PID settings)
    - Sinusoidal Startup
    - Brake
    - Servo Settings

### 3.2 ESC Selection UI Behavior
- **Default state:** All ESCs selected (green borders)
- **Click to deselect:** Removes ESC from selection (no border)
- **Mismatched settings:** Red border indicates ESC differs from first ESC
- **Unsaved changes:** Purple border for ESCs with pending changes
- **Missing ESC:** Red border with loading indicator

## Phase 4: Clean Up ActuatorComponent

### 4.1 Remove AM32 Integration
- Remove lines 322-351 (AM32 ESC Settings section)
- Remove `am32SettingsComponent` Component definition
- Remove `am32SettingsLoader` Loader
- Clean up any AM32-related imports

## Phase 5: Update Build System

### 5.1 Update CMakeLists.txt or .pri files
- Add `ESCComponent.h` and `ESCComponent.cc` to source lists
- Add `ESCComponent.qml` and `ESCComponentSummary.qml` to QML resource lists
- Ensure proper MOC generation for new classes

### 5.2 Update qgroundcontrol.qrc
- Add entries for new QML files:
  - `/qml/ESCComponent.qml`
  - `/qml/ESCComponentSummary.qml`

## Phase 6: UI Implementation Details

### 6.1 ESC Grid Component
- Display ESCs in horizontal row at top
- Each ESC shows:
  - ESC number
  - Selection state (border color)
  - Loading indicator when reading/writing
- Border states:
  - Green: Selected and synced
  - None: Not selected
  - Red: Mismatched settings or missing
  - Purple: Unsaved changes

### 6.2 Settings Layout
- Use QGCGroupBox for each setting category
- Grid layout within groups (1-3 columns based on content)
- Conditional visibility based on firmware/EEPROM version
- Disable dependent fields when parent setting is off

### 6.3 Field Components
- Number fields with min/max/step validation
- Select dropdowns for enums
- Toggle switches for boolean settings
- Display units and current values
- Apply display factors and offsets as needed

## Implementation Order

1. **Create new component files** (ESCComponent.h/.cc/.qml)
2. **Register component** in PX4AutoPilotPlugin
3. **Update AM32SettingsComponent** to match AM32 Configurator UI
4. **Remove from ActuatorComponent**
5. **Update build system**

## Success Criteria

✅ ESC configuration has its own dedicated page in vehicle setup
✅ AM32 settings UI matches AM32 Configurator design
✅ ESC selection behavior works as specified (green/red/purple borders)
✅ All setting groups are properly organized
✅ ActuatorComponent is cleaner without ESC configuration

## Notes

- Keep AM32SettingsComponent as separate QML for AM32-specific UI
- ESCComponent provides the generic container for future ESC protocols
- AM32EepromFactGroup and MAVLink handling remain unchanged
- ESC detection must be robust to prevent crashes
- UI should closely match AM32 Configurator for user familiarity
