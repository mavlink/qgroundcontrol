# QtCreator Plugin Tools for QGroundControl Development

This directory contains tools and configuration for improving the QGC development experience, including IDE support for QGC-specific patterns, code generation, and static analysis.

## Overview

QGroundControl development involves several cross-cutting patterns (Fact System, MAVLink, QML bindings) that require synchronized changes across multiple files. These tools help by:

- **Reducing boilerplate** by 70-80% for common patterns
- **Catching errors at edit-time** instead of runtime
- **Improving navigation** between related code artifacts
- **Generating code** from specifications

## Directory Structure

```
qtcreator-plugin/
├── README.md                      # This file
├── schemas/
│   └── factmetadata.schema.json   # JSON Schema for FactMetaData validation
├── snippets/
│   └── qgc-cpp.xml                # QtCreator C++ snippets
└── locators/
    ├── qgc_locator.py             # CLI search tool
    └── README.md                  # Locator usage docs
```

---

## Implemented Features

### 1. JSON Schema Validation (VS Code)

Automatic validation for `*Fact.json` files in VS Code.

**Already configured in `.vscode/settings.json`** - just open a Fact JSON file to see validation.

**Features:**
- Red squiggles on invalid JSON keys
- Autocomplete for `type`, `units`, `enumStrings`
- Hover documentation for field meanings

### 2. QtCreator Snippets

Pre-built snippets for common QGC patterns.

**Installation:**
```bash
# Linux
cp snippets/qgc-cpp.xml ~/.config/QtProject/qtcreator/snippets/

# macOS
cp snippets/qgc-cpp.xml ~/Library/Application\ Support/QtProject/qtcreator/snippets/

# Windows
copy snippets\qgc-cpp.xml %APPDATA%\QtProject\qtcreator\snippets\
```

**Available snippets (type trigger + Tab):**

| Trigger | Description |
|---------|-------------|
| `factprop` | Q_PROPERTY declaration for a Fact |
| `factmember` | Fact member variable with initialization |
| `factaccessor` | Fact getter method |
| `factinit` | _addFact() call in constructor |
| `factfull` | Complete Fact declaration (all parts) |
| `mavhandle` | MAVLink message handler function |
| `mavswitch` | switch on message.msgid |
| `mavcase` | Single case for MAVLink switch |
| `mavdecode` | MAVLink message decode pattern |
| `vehiclenull` | Safe vehicle null check pattern |
| `vehiclenullv` | Safe vehicle null check (void return) |
| `paramnull` | Safe parameter access with null check |
| `factgroupctor` | FactGroup constructor |
| `handlemsg` | handleMessage() override |

### 3. Vehicle Null-Check Analyzer

Static analysis to detect unsafe `activeVehicle()` access patterns.

**Usage:**
```bash
# Analyze specific files
python3 tools/analyzers/vehicle_null_check.py src/Vehicle/*.cc

# Analyze directory
python3 tools/analyzers/vehicle_null_check.py src/

# Integrated with pre-commit (runs automatically)
pre-commit run vehicle-null-check --all-files
```

**Detects:**
- `activeVehicle()->method()` without prior null check
- `getParameter()` result used without validation

### 4. QGC Locator CLI

Search for Facts, FactGroups, and MAVLink messages from command line.

**Usage:**
```bash
# Find Fact names containing 'lat'
python3 tools/qtcreator-plugin/locators/qgc_locator.py fact lat

# Find FactGroup classes with 'gps'
python3 tools/qtcreator-plugin/locators/qgc_locator.py factgroup gps

# Find MAVLink message usage
python3 tools/qtcreator-plugin/locators/qgc_locator.py mavlink HEARTBEAT

# Find parameters in JSON metadata
python3 tools/qtcreator-plugin/locators/qgc_locator.py param BATT
```

**Output format:** `name<TAB>path:line` (for editor integration)

See `locators/README.md` for editor integration instructions.

### 5. FactGroup Generator

Generate complete FactGroup boilerplate from a specification.

**Usage:**
```bash
# Generate with dry-run preview
python -m tools.generators.factgroup.cli \
  --name Example \
  --facts "temp:double:degC,pressure:double:Pa" \
  --dry-run

# Generate with MAVLink handlers
python -m tools.generators.factgroup.cli \
  --name Wind \
  --facts "direction:double:deg,speed:double:m/s,verticalSpeed:double:m/s" \
  --mavlink "WIND_COV,HIGH_LATENCY2" \
  --output src/Vehicle/FactGroups/
```

**Generates:**
- `VehicleExampleFactGroup.h` - Header with Q_PROPERTY, accessors, members
- `VehicleExampleFactGroup.cc` - Implementation with constructor, handlers
- `ExampleFact.json` - FactMetaData JSON

**Requires:** `pip install jinja2`

---

## External Tools Integration (QtCreator)

Add via **Tools → External → Configure**:

### MAVLink Message Lookup
- **Executable**: `xdg-open`
- **Arguments**: `https://mavlink.io/en/messages/common.html#%{CurrentSelection}`

### QGC Fact Locator
- **Executable**: `python3`
- **Arguments**: `%{CurrentProject:Path}/tools/qtcreator-plugin/locators/qgc_locator.py fact %{CurrentDocument:Selection}`

### QGC MAVLink Locator
- **Executable**: `python3`
- **Arguments**: `%{CurrentProject:Path}/tools/qtcreator-plugin/locators/qgc_locator.py mavlink %{CurrentDocument:Selection}`

---

## MAVLink Scaling Factors Reference

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

---

## Related Tools

- **`tools/analyzers/`** - Static analysis scripts (vehicle null-check)
- **`tools/generators/`** - Code generators (FactGroup)

## Resources

### QtCreator Plugin Development
- [Creating C++-Based Plugins](https://doc.qt.io/qtcreator-extending/first-plugin.html)
- [Lua Extensions](https://doc.qt.io/qtcreator-extending/index.html)

### QGC Architecture
- [Fact System Documentation](../../docs/en/qgc-dev-guide/fact_system.md)
- [Communication Flow](../../docs/en/qgc-dev-guide/communication_flow.md)
