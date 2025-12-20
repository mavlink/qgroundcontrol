# QtCreator Plugin for QGroundControl Development

This document proposes a QtCreator plugin to improve the QGC development experience by providing IDE support for QGC-specific patterns, code generation, and static analysis.

## Executive Summary

QGroundControl development involves several cross-cutting patterns (Fact System, MAVLink, QML bindings) that require synchronized changes across multiple files. A dedicated QtCreator plugin could:

- **Reduce boilerplate** by 70-80% for common patterns
- **Catch errors at edit-time** instead of runtime
- **Improve navigation** between related code artifacts
- **Generate code** from MAVLink definitions and metadata

## Pain Points Analysis

### 1. Fact System Complexity

The Fact System is QGC's core abstraction for vehicle parameters. Adding a new FactGroup requires synchronized changes in 4+ files:

| File | Required Changes |
|------|------------------|
| `NewFactGroup.h` | Q_PROPERTY declarations, getters, private members |
| `NewFactGroup.cc` | Constructor with `_addFact()` calls, message handlers |
| `NewFactGroup.FactMetaData.json` | Parameter metadata (type, units, range, enums) |
| `Vehicle.h` | Property, getter, member, name constant |
| `Vehicle.cc` | Initialization, `_addFactGroup()` registration |

**Current issues:**
- Missing any step causes silent runtime failures
- No IDE validation of JSON metadata
- Magic string parameter names break silently on refactor
- 16+ existing FactGroups follow identical boilerplate

### 2. MAVLink Message Handling

Each FactGroup implements `handleMessage()` with repetitive decode patterns:

```cpp
void VehicleGPSFactGroup::handleMessage(Vehicle*, const mavlink_message_t& message) {
    switch (message.msgid) {
    case MAVLINK_MSG_ID_GPS_RAW_INT: {
        mavlink_gps_raw_int_t gpsRawInt;
        mavlink_msg_gps_raw_int_decode(&message, &gpsRawInt);
        _latFact.setRawValue(gpsRawInt.lat * 1e-7);  // Scaling factor
        _lonFact.setRawValue(gpsRawInt.lon * 1e-7);
        break;
    }
    // ... more handlers
    }
}
```

**Current issues:**
- No autocomplete for `MAVLINK_MSG_ID_*` constants
- Scaling factors (1e-7, /100, etc.) are error-prone
- No validation that decoded struct matches message ID
- Generated MAVLink headers not indexed until after build

### 3. Parameter Access Safety

Vehicle parameter access requires defensive null-checking:

```cpp
// Unsafe - crashes if no vehicle connected
auto* v = MultiVehicleManager::instance()->activeVehicle();
v->armed();  // Potential nullptr dereference

// Safe pattern (7 lines for 1 line of logic)
auto* v = MultiVehicleManager::instance()->activeVehicle();
if (!v) return;
Fact* param = v->parameterManager()->getParameter(-1, "PARAM_NAME");
if (!param) { qCWarning(Log) << "Parameter not found"; return; }
param->setCookedValue(newValue);
```

**Current issues:**
- Easy to forget null-checks
- String-based parameter names have no compile-time validation
- No IDE warning for unsafe access patterns

### 4. FactMetaData JSON Validation

Metadata files define parameter properties but have no schema validation:

```json
{
  "QGC.MetaData.Facts": [
    {
      "name": "distance",
      "typ": "double",      // Typo: should be "type" - fails silently
      "units": "deg",
      "min": 0,
      "max": 360
    }
  ]
}
```

**Current issues:**
- Typos in JSON keys fail at runtime
- No autocomplete for valid types/units
- Type mismatches between JSON and C++ not detected

---

## Proposed Plugin Features

### Feature 1: FactMetaData JSON Support

**Capability:** Schema validation and editing support for `*.FactMetaData.json` files.

**Implementation:**
- Register JSON Schema for validation
- Provide autocomplete for `type`, `units`, `enumStrings`
- Quick-fix: Generate C++ Fact member from JSON definition
- Navigation: Jump between JSON metadata and C++ implementation

**JSON Schema:**
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "QGC FactMetaData",
  "type": "object",
  "properties": {
    "version": { "type": "integer" },
    "QGC.MetaData.Facts": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["name", "type"],
        "properties": {
          "name": { "type": "string", "description": "Fact identifier" },
          "type": {
            "enum": ["uint8", "int8", "uint16", "int16", "uint32", "int32",
                     "uint64", "int64", "float", "double", "string", "bool",
                     "elapsedTimeInSeconds", "custom"],
            "description": "Value type"
          },
          "units": { "type": "string", "description": "Display units" },
          "min": { "type": "number", "description": "Minimum valid value" },
          "max": { "type": "number", "description": "Maximum valid value" },
          "default": { "description": "Default value" },
          "decimalPlaces": { "type": "integer", "description": "Display precision" },
          "enumStrings": {
            "type": "array",
            "items": { "type": "string" },
            "description": "Display strings for enum values"
          },
          "enumValues": {
            "type": "array",
            "items": { "type": "number" },
            "description": "Numeric values for enums"
          },
          "shortDesc": { "type": "string", "description": "Brief description" },
          "longDesc": { "type": "string", "description": "Detailed description" }
        }
      }
    }
  }
}
```

**User experience:**
```
✓ Red squiggles on invalid JSON keys
✓ Autocomplete: "type": "█" → shows valid types
✓ Hover on "units" shows: "Common: m, m/s, deg, rad, %, degE7"
✓ Ctrl+Click on Fact name → jumps to C++ declaration
```

### Feature 2: FactGroup Generation Wizard

**Capability:** File → New → QGC FactGroup wizard that generates all required files.

**Wizard UI:**
```
┌─────────────────────────────────────────────────────┐
│ New QGC FactGroup                                   │
├─────────────────────────────────────────────────────┤
│ Class Name: [VehicleGPSFactGroup    ]               │
│ Display Name: [GPS                   ]              │
│                                                     │
│ MAVLink Messages:                                   │
│ [x] GPS_RAW_INT                                     │
│ [x] GPS_STATUS                                      │
│ [ ] GLOBAL_POSITION_INT                             │
│                                                     │
│ Facts (auto-populated from MAVLink):                │
│ ┌─────────┬────────┬───────┬─────────┬───────────┐ │
│ │ Name    │ Type   │ Units │ Source  │ Scaling   │ │
│ ├─────────┼────────┼───────┼─────────┼───────────┤ │
│ │ lat     │ double │ deg   │ lat     │ * 1e-7    │ │
│ │ lon     │ double │ deg   │ lon     │ * 1e-7    │ │
│ │ alt     │ double │ m     │ alt     │ / 1000.0  │ │
│ │ hdop    │ double │ -     │ eph     │ / 100.0   │ │
│ │ vdop    │ double │ -     │ epv     │ / 100.0   │ │
│ │ course  │ double │ deg   │ cog     │ / 100.0   │ │
│ │ speed   │ double │ m/s   │ vel     │ / 100.0   │ │
│ └─────────┴────────┴───────┴─────────┴───────────┘ │
│                              [+ Add Custom Fact]    │
│                                                     │
│ Options:                                            │
│ [x] Generate MAVLink message handlers               │
│ [x] Register in Vehicle.h/cc                        │
│ [x] Create unit test stub                           │
│ [ ] Generate QML test component                     │
│                                                     │
│                        [Cancel]  [Create FactGroup] │
└─────────────────────────────────────────────────────┘
```

**Generated files:**

`VehicleGPSFactGroup.h`:
```cpp
#pragma once

#include "FactGroup.h"
#include "QGCMAVLink.h"

class VehicleGPSFactGroup : public FactGroup
{
    Q_OBJECT
    Q_MOC_INCLUDE("Fact.h")

public:
    VehicleGPSFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* lat     READ lat     CONSTANT)
    Q_PROPERTY(Fact* lon     READ lon     CONSTANT)
    Q_PROPERTY(Fact* alt     READ alt     CONSTANT)
    Q_PROPERTY(Fact* hdop    READ hdop    CONSTANT)
    Q_PROPERTY(Fact* vdop    READ vdop    CONSTANT)
    Q_PROPERTY(Fact* course  READ course  CONSTANT)
    Q_PROPERTY(Fact* speed   READ speed   CONSTANT)

    Fact* lat()    { return &_latFact; }
    Fact* lon()    { return &_lonFact; }
    Fact* alt()    { return &_altFact; }
    Fact* hdop()   { return &_hdopFact; }
    Fact* vdop()   { return &_vdopFact; }
    Fact* course() { return &_courseFact; }
    Fact* speed()  { return &_speedFact; }

    void handleMessage(Vehicle* vehicle, const mavlink_message_t& message) override;

    static const char* _gpsFactGroupName;

private:
    void _handleGpsRawInt(const mavlink_message_t& message);
    void _handleGpsStatus(const mavlink_message_t& message);

    Fact _latFact;
    Fact _lonFact;
    Fact _altFact;
    Fact _hdopFact;
    Fact _vdopFact;
    Fact _courseFact;
    Fact _speedFact;
};
```

### Feature 3: MAVLink Integration

**Capability:** Autocomplete and code generation for MAVLink message handling.

**Autocomplete in switch statements:**
```cpp
void MyFactGroup::handleMessage(Vehicle*, const mavlink_message_t& message) {
    switch (message.msgid) {
    case MAVLINK_MSG_ID_█
    // Autocomplete shows:
    //   MAVLINK_MSG_ID_GPS_RAW_INT (24)
    //   MAVLINK_MSG_ID_GPS_STATUS (25)
    //   MAVLINK_MSG_ID_GLOBAL_POSITION_INT (33)
    //   ...
    }
}
```

**Code generation on selection:**
```cpp
case MAVLINK_MSG_ID_GPS_RAW_INT: {
    mavlink_gps_raw_int_t gpsRawInt;
    mavlink_msg_gps_raw_int_decode(&message, &gpsRawInt);
    // Available fields: time_usec, lat, lon, alt, eph, epv, vel, cog, fix_type, satellites_visible
    break;
}
```

**Hover documentation:**
```
┌────────────────────────────────────────────────────┐
│ mavlink_gps_raw_int_t.lat                          │
│ ──────────────────────────────────────────────────│
│ Type: int32_t                                      │
│ Description: Latitude (WGS84, EGM96 ellipsoid)     │
│ Units: degE7 (multiply by 1e-7 for degrees)        │
│ Range: -900000000 to 900000000                     │
└────────────────────────────────────────────────────┘
```

### Feature 4: Vehicle Null-Check Inspector

**Capability:** Static analysis to detect unsafe `activeVehicle()` access.

**Detection patterns:**
```cpp
// WARNING: Potential null pointer dereference
auto* v = MultiVehicleManager::instance()->activeVehicle();
v->armed();  // ⚠ No null check before use

// WARNING: Parameter access without validation
Fact* param = vehicle->parameterManager()->getParameter(-1, "BATT_CAPACITY");
param->rawValue();  // ⚠ getParameter can return nullptr
```

**Quick-fix suggestions:**
```cpp
// Quick-fix: Add null guard
if (!v) {
    return;  // or: return defaultValue;
}
v->armed();

// Quick-fix: Add parameter validation
if (!param) {
    qCWarning(VehicleLog) << "Parameter BATT_CAPACITY not found";
    return;
}
```

### Feature 5: QGC Locator Filters

**Capability:** Quick navigation via Ctrl+K with QGC-specific filters.

| Prefix | Searches | Example |
|--------|----------|---------|
| `f ` | Fact names across all FactGroups | `f lat` → finds GPS lat, home lat |
| `p ` | Parameter names from metadata | `p BATT` → BATT_CAPACITY, BATT_LOW_VOLT |
| `m ` | MAVLink message IDs | `m GPS` → GPS_RAW_INT, GPS_STATUS |
| `fg ` | FactGroup classes | `fg gps` → VehicleGPSFactGroup |
| `qc ` | QML components | `qc slider` → FactSlider.qml |

### Feature 6: Parameter Refactoring

**Capability:** Safe rename of parameters across JSON, C++, and QML.

**Before rename:**
```cpp
// C++
getParameter(-1, "BATT_CAPACITY")

// JSON
{ "name": "BATT_CAPACITY", "type": "int32" }

// QML
fact: controller.getParameterFact(-1, "BATT_CAPACITY")
```

**Refactoring action:** Rename "BATT_CAPACITY" → "BAT_CAPACITY"

**After rename (all files updated):**
```cpp
// C++
getParameter(-1, "BAT_CAPACITY")

// JSON
{ "name": "BAT_CAPACITY", "type": "int32" }

// QML
fact: controller.getParameterFact(-1, "BAT_CAPACITY")
```

---

## Implementation Options

### Option A: Full C++ Plugin (Recommended for full feature set)

**Structure:**
```
qgc-qtcreator-plugin/
├── CMakeLists.txt
├── qgcplugin.h/cc              # IPlugin implementation
├── factmetadata/
│   ├── factjsoneditor.h/cc     # Custom JSON editor with schema
│   ├── factjsonhighlighter.h/cc
│   └── factschema.json
├── wizard/
│   ├── factgroupwizard.h/cc    # New file wizard
│   ├── factgroupwizard.json    # Wizard template
│   └── templates/              # Code templates
├── mavlink/
│   ├── mavlinkcompletion.h/cc  # Code completion provider
│   ├── mavlinkparser.h/cc      # Parse MAVLink XML definitions
│   └── mavlinkhover.h/cc       # Hover documentation
├── inspections/
│   ├── vehiclenullcheck.h/cc   # Null-check analysis
│   └── factusagecheck.h/cc     # Fact usage patterns
└── locator/
    ├── factlocator.h/cc        # "f " filter
    ├── paramlocator.h/cc       # "p " filter
    └── mavlinklocator.h/cc     # "m " filter
```

**Pros:**
- Full access to QtCreator API
- Best performance
- Can implement all features

**Cons:**
- Must rebuild per QtCreator version (ABI compatibility)
- Higher development effort
- Requires Qt Creator source or development headers

### Option B: Lua Extension (Simpler, cross-version)

```lua
-- qgc-extension/init.lua
local a = require("async")
local fetch = require("Fetch").fetch
local locator = require("Locator")

-- Register locator filter for Facts
locator.addFilter({
    id = "qgc-facts",
    displayName = "QGC Facts",
    prefix = "f ",
    search = function(query)
        -- Search fact names in JSON files
        return findFactsMatching(query)
    end
})

-- Register JSON schema for FactMetaData
local lsp = require("LSP")
lsp.registerJsonSchema({
    filePattern = "*.FactMetaData.json",
    schemaPath = "qgc-extension/schemas/factmetadata.json"
})
```

**Pros:**
- Works across QtCreator versions
- Simpler development
- No compilation needed

**Cons:**
- Limited API (no custom wizards, limited static analysis)
- Less performant for complex features

### Option C: Hybrid Approach (Pragmatic)

Implement features using the most appropriate technology:

| Feature | Technology | Rationale |
|---------|------------|-----------|
| JSON Schema validation | Built-in + schema file | Zero plugin code needed |
| Locator filters | Lua extension | Simple, cross-version |
| Code snippets | QtCreator snippets | Built-in feature |
| FactGroup wizard | C++ plugin | Requires full API |
| MAVLink completion | C++ plugin | Performance-critical |
| Null-check analysis | clang-tidy check | Leverage existing tooling |

---

## Quick Wins (No Plugin Required)

### 1. JSON Schema for FactMetaData

Add to project root as `schemas/factmetadata.schema.json` and configure QtCreator:

**Edit → Preferences → JSON → Add Schema:**
- Pattern: `*.FactMetaData.json`
- Schema: `schemas/factmetadata.schema.json`

### 2. Code Snippets

**Edit → Preferences → Text Editor → Snippets → C++ → Add:**

**Snippet: `factprop`**
```cpp
Q_PROPERTY(Fact* $name$ READ $name$ CONSTANT)
Fact* $name$() { return &_$name$Fact; }
```

**Snippet: `factmember`**
```cpp
Fact _$name$Fact;
```

**Snippet: `mavhandle`**
```cpp
case MAVLINK_MSG_ID_$MSGNAME$: {
    mavlink_$msgname$_t $msgname$;
    mavlink_msg_$msgname$_decode(&message, &$msgname$);
    $END$
    break;
}
```

**Snippet: `vehiclenull`**
```cpp
Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
if (!vehicle) {
    return$END$;
}
```

### 3. Live Templates for Common Patterns

Create file templates in QtCreator:

**File → New → Import Template → FactGroup.h:**
```cpp
#pragma once

#include "FactGroup.h"
#include "QGCMAVLink.h"

class %{ClassName} : public FactGroup
{
    Q_OBJECT

public:
    %{ClassName}(QObject* parent = nullptr);

    // Add Q_PROPERTY declarations here

    void handleMessage(Vehicle* vehicle, const mavlink_message_t& message) override;

    static const char* _%{CN:l}FactGroupName;

private:
    // Add handler methods here
    // Add Fact members here
};
```

### 4. External Tools Integration

**Edit → Preferences → Environment → External Tools → Add:**

**MAVLink Message Lookup:**
- Executable: `xdg-open`
- Arguments: `https://mavlink.io/en/messages/common.html#%{CurrentSelection}`

**Parameter Documentation:**
- Executable: `xdg-open`
- Arguments: `https://ardupilot.org/copter/docs/parameters.html#%{CurrentSelection}`

---

## Development Roadmap

### Phase 1: Foundation (2-3 weeks)
- [ ] Create JSON Schema for FactMetaData validation
- [ ] Add code snippets for common patterns
- [ ] Document QtCreator configuration for QGC development
- [ ] Set up plugin development environment

### Phase 2: Lua Extension (2-3 weeks)
- [ ] Implement locator filters (f, p, m, fg prefixes)
- [ ] Add external tool integrations
- [ ] Create file templates for FactGroup, QML components

### Phase 3: C++ Plugin Core (4-6 weeks)
- [ ] Plugin skeleton with proper CMake setup
- [ ] FactGroup generation wizard
- [ ] JSON editor with schema validation and autocomplete

### Phase 4: MAVLink Integration (3-4 weeks)
- [ ] MAVLink XML parser
- [ ] Message ID autocomplete in switch statements
- [ ] Hover documentation for MAVLink types
- [ ] Code generation for message handlers

### Phase 5: Static Analysis (3-4 weeks)
- [ ] Vehicle null-check inspector
- [ ] Fact usage pattern validation
- [ ] Parameter name consistency checking

### Phase 6: Advanced Features (ongoing)
- [ ] Cross-file refactoring
- [ ] Test generation
- [ ] QML/C++ binding validation

---

## Resources

### QtCreator Plugin Development
- [Creating C++-Based Plugins](https://doc.qt.io/qtcreator-extending/first-plugin.html)
- [C++ API Reference](https://doc.qt.io/qtcreator-extending/qtcreator-api.html)
- [Lua Extensions](https://doc.qt.io/qtcreator-extending/index.html)

### Reference Implementations
- [ROS Qt Creator Plugin](https://github.com/ros-industrial/ros_qtc_plugin) - Domain-specific IDE integration
- [QodeAssist](https://github.com/Palm1r/QodeAssist) - AI integration example

### QGC Architecture
- [Fact System Documentation](../docs/en/qgc-dev-guide/fact_system.md)
- [Communication Flow](../docs/en/qgc-dev-guide/communication_flow.md)
- [MAVLink File Formats](../docs/en/qgc-dev-guide/file_formats/mavlink.md)

---

## Appendix: MAVLink Scaling Factors Reference

Common scaling factors used in QGC MAVLink handlers:

| Field Pattern | Scaling | Example |
|---------------|---------|---------|
| `lat`, `lon` (degE7) | `* 1e-7` | `gpsRawInt.lat * 1e-7` |
| `alt` (mm) | `/ 1000.0` | `gpsRawInt.alt / 1000.0` |
| `eph`, `epv` (cm) | `/ 100.0` | `gpsRawInt.eph / 100.0` |
| `vel` (cm/s) | `/ 100.0` | `gpsRawInt.vel / 100.0` |
| `cog` (cdeg) | `/ 100.0` | `gpsRawInt.cog / 100.0` |
| `hdg` (cdeg) | `/ 100.0` | `vfrHud.heading / 100.0` |
| `press_abs` (hPa) | `* 100.0` (Pa) | `scaledPressure.press_abs * 100.0` |
| `temperature` (cdegC) | `/ 100.0` | `scaledPressure.temperature / 100.0` |
| `roll`, `pitch`, `yaw` (rad) | `* (180.0/M_PI)` | degrees conversion |
| `rollspeed`, etc. (rad/s) | `* (180.0/M_PI)` | degrees/s conversion |

---

## Contributing

If you're interested in contributing to this plugin:

1. Join the [QGC Discord](https://discord.gg/qgroundcontrol) #development channel
2. Review the [Contributing Guidelines](../.github/CONTRIBUTING.md)
3. Start with Phase 1 quick wins to understand the patterns
4. Propose specific features in GitHub Issues before implementing
