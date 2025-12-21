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
qtcreator/
├── README.md                      # This file
├── snippets/
│   └── qgc-cpp.xml                # QtCreator C++ snippets
├── lua/
│   └── QGCTools/                  # QtCreator Lua extension (14+)
│       ├── QGCTools.lua           # Extension spec
│       └── init.lua               # Main implementation
└── plugin/                        # Native C++ plugin
    ├── CMakeLists.txt             # Plugin build config
    └── src/                       # Plugin source code
```

**Related tools:**
- `tools/lsp/` - Language Server for real-time diagnostics
- `tools/schemas/` - JSON schemas for validation
- `tools/locators/` - CLI search tools

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

### 3. QtCreator Lua Extension (Qt Creator 14+)

Full IDE integration with menu actions, keyboard shortcuts, and dialogs.

**Requirements:**
- Qt Creator 14.0.0 or later (Lua extension support)
- Python 3.10+

**Installation:**
```bash
# Linux
mkdir -p ~/.local/share/QtProject/qtcreator/lua/extensions
cp -r lua/QGCTools ~/.local/share/QtProject/qtcreator/lua/extensions/

# macOS
mkdir -p ~/Library/Application\ Support/QtProject/qtcreator/lua/extensions
cp -r lua/QGCTools ~/Library/Application\ Support/QtProject/qtcreator/lua/extensions/

# Windows
xcopy /E /I lua\QGCTools %APPDATA%\QtProject\qtcreator\lua\extensions\QGCTools
```

Restart Qt Creator after installation. Access via **Tools → QGC**.

**Features:**

| Menu Action | Shortcut | Description |
|-------------|----------|-------------|
| Search Facts... | `Ctrl+Shift+F` | Search for Fact names in codebase |
| Search FactGroups... | `Ctrl+Shift+G` | Search for FactGroup classes |
| Search MAVLink Usage... | `Ctrl+Shift+M` | Find MAVLink message handlers |
| Check Null Safety (File) | `Ctrl+Shift+N` | Analyze current file for null issues |
| Check Null Safety (Project) | - | Analyze entire src/ directory |
| Generate FactGroup... | `Ctrl+Shift+Alt+G` | Interactive FactGroup generator wizard |

**Workflow:**
1. Press shortcut or use menu
2. Enter search query or generator parameters
3. Results shown in picker dialog
4. Click result to open file at line

**LSP Integration (Qt Creator 14+):**

The Lua extension automatically registers the QGC Language Server for real-time diagnostics:
- Warnings appear inline as you type (no manual analysis needed)
- Detects `null-vehicle` and `null-parameter` issues
- Requires: `pip install pygls lsprotocol`

If LSP setup fails, the menu-based analyzers still work as a fallback.

### 4. Vehicle Null-Check Analyzer (CLI)

Static analysis to detect unsafe `activeVehicle()` access patterns.

**Usage:**
```bash
# Analyze specific files
python3 tools/analyzers/vehicle_null_check.py src/Vehicle/*.cc

# Analyze directory
python3 tools/analyzers/vehicle_null_check.py src/

# JSON output for CI/editor integration
python3 tools/analyzers/vehicle_null_check.py --json src/

# Integrated with pre-commit (runs automatically)
pre-commit run vehicle-null-check --all-files
```

**Detects:**
- `activeVehicle()->method()` without prior null check
- `getParameter()` result used without validation

**Output includes fix suggestions** for each violation found.

### 5. QGC Locator CLI

Search for Facts, FactGroups, and MAVLink messages from command line.

**Usage:**
```bash
# Find Fact names containing 'lat'
python3 tools/locators/qgc_locator.py fact lat

# Find FactGroup classes with 'gps'
python3 tools/locators/qgc_locator.py factgroup gps

# Find MAVLink message usage
python3 tools/locators/qgc_locator.py mavlink HEARTBEAT

# Find parameters in JSON metadata
python3 tools/locators/qgc_locator.py param BATT

# JSON output for programmatic use
python3 tools/locators/qgc_locator.py --json fact lat

# Limit results
python3 tools/locators/qgc_locator.py --limit 10 fact lat
```

**Output format:** `name<TAB>path:line` (for editor integration)

See `tools/locators/README.md` for editor integration instructions.

### 6. FactGroup Generator

Generate complete FactGroup boilerplate from a specification.

**Usage with YAML spec (recommended):**
```bash
# Validate spec without generating
python3 -m tools.generators.factgroup.cli --spec wind.yaml --validate

# Preview generated files
python3 -m tools.generators.factgroup.cli --spec wind.yaml --dry-run

# Generate files
python3 -m tools.generators.factgroup.cli \
  --spec tools/generators/factgroup/examples/wind.yaml \
  --output src/Vehicle/FactGroups/
```

**Usage with CLI arguments:**
```bash
python3 -m tools.generators.factgroup.cli \
  --name Wind \
  --facts "direction:double:deg,speed:double:m/s" \
  --mavlink "WIND_COV,HIGH_LATENCY2" \
  --output src/Vehicle/FactGroups/
```

**Example YAML spec** (`tools/generators/factgroup/examples/wind.yaml`):
```yaml
domain: Wind
update_rate_ms: 1000
facts:
  - name: direction
    type: double
    units: deg
    short_desc: Wind direction
    min: 0
    max: 360
  - name: speed
    type: double
    units: m/s
mavlink_messages:
  - WIND_COV
```

**Generates:**
- `VehicleWindFactGroup.h` - Header with Q_PROPERTY, accessors, members
- `VehicleWindFactGroup.cc` - Implementation with constructor, handlers
- `WindFact.json` - FactMetaData JSON

**Requires:** `pip install jinja2` (and optionally `pyyaml` for YAML specs)

---

## External Tools Integration (QtCreator)

Add via **Tools → External → Configure**:

### MAVLink Message Lookup
- **Executable**: `xdg-open`
- **Arguments**: `https://mavlink.io/en/messages/common.html#%{CurrentSelection}`

### QGC Fact Locator
- **Executable**: `python3`
- **Arguments**: `%{CurrentProject:Path}/tools/locators/qgc_locator.py fact %{CurrentDocument:Selection}`

### QGC MAVLink Locator
- **Executable**: `python3`
- **Arguments**: `%{CurrentProject:Path}/tools/locators/qgc_locator.py mavlink %{CurrentDocument:Selection}`

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
