# Generated Vehicle Config Pages

This page explains how the Vehicle Setup pages (Power, Safety, etc.) are generated from JSON definitions and how to extend them.

## Architecture Overview

The generation pipeline is:

1. **JSON page definitions** in `src/AutoPilotPlugins/{APM,PX4}/VehicleConfig/*.VehicleConfig.json`
2. **Python generator** in `tools/generators/config_qml/page_generator.py`
3. **Generated QML** in `build/src/AutoPilotPlugins/{APM,PX4}/generated/`
4. **C++ component class** (e.g. `APMPowerComponent`) loads the generated QML via `setupSource()`

At build time, CMake runs the generator for each firmware plugin and places generated QML files in the build tree.
Those files are compiled into the corresponding `QGroundControl.AutoPilotPlugins.*` QML module.

## Where Generation Is Wired

Generation is configured in each firmware plugin's `CMakeLists.txt`:

- `src/AutoPilotPlugins/APM/CMakeLists.txt` — target `GenerateConfigQmlAPM`
- `src/AutoPilotPlugins/PX4/CMakeLists.txt` — target `GenerateConfigQmlPX4`

Each runs:

```bash
python -m tools.generators.config_qml.generate_pages \
    --pages-dir <pages_dir> \
    --output-dir <build_gen_dir>
```

The generator entry point is `tools/generators/config_qml/generate_pages.py`, with most logic in `tools/generators/config_qml/page_generator.py`.

## JSON File Structure

Each `*.VehicleConfig.json` file has this top-level structure (all fields except `version` and `sections` are optional):

```jsonc
{
    "version": 1,
    "imports": ["QGroundControl.AutoPilotPlugins.PX4"],
    "controllerType": "PowerComponentController",
    "constants": { ... },
    "params": { ... },
    "bindings": { ... },
    "sections": [ ... ]
}
```

| Field | Purpose |
|---|---|
| `version` | Schema version (currently `1`) |
| `imports` | Extra QML import statements (e.g. firmware-specific modules) |
| `controllerType` | QML type for the `FactPanelController` (default: `FactPanelController`) |
| `constants` | Named literal values → `readonly property var` |
| `params` | Named parameter lookups → `property var` via `controller.getParameterFact()` |
| `bindings` | Named QML expressions → `property var` (arbitrary expressions) |
| `sections` | Array of section definitions (see below) |

### Constants

Constants are emitted as `readonly property var` in the generated QML.
Values should be native JSON numbers (not strings):

```json
"constants": {
    "_monitorDisabled":  0,
    "_monitorAnalogVoltageOnly": 3,
    "_monitorAnalogVoltageAndCurrent": 4
}
```

Generated output:

```qml
readonly property var _monitorDisabled: 0
readonly property var _monitorAnalogVoltageOnly: 3
readonly property var _monitorAnalogVoltageAndCurrent: 4
```

### Params

The `params` section declares vehicle parameter lookups.
Each entry generates a `property var` that calls `controller.getParameterFact()` or `controller.parameterExists()`.

Three forms are supported:

| Form | JSON | Generated QML |
|---|---|---|
| Optional (default) | `"_param": "PARAM_NAME"` | `property var _param: controller.getParameterFact(-1, "PARAM_NAME", false)` |
| Required | `"_param": { "name": "PARAM_NAME", "required": true }` | `property var _param: controller.getParameterFact(-1, "PARAM_NAME")` |
| Exists-only | `"_param": { "name": "PARAM_NAME", "existsOnly": true }` | `property var _param: controller.parameterExists(-1, "PARAM_NAME")` |

Example:

```json
"params": {
    "_uavcanEnable":    "UAVCAN_ENABLE",
    "_uavcanAvailable": { "name": "UAVCAN_ENABLE", "existsOnly": true },
    "_fsGcsEnable":     { "name": "FS_GCS_ENABLE", "required": true }
}
```

### Bindings

Bindings are arbitrary QML expressions emitted as `property var`.
Use these for derived values that cannot be expressed as simple param lookups:

```json
"bindings": {
    "_anyBattFailsafe": "_battLowActParam.value !== 0 || _battCrtActParam.value !== 0"
}
```

Generated output:

```qml
property var _anyBattFailsafe: _battLowActParam.value !== 0 || _battCrtActParam.value !== 0
```

## Sections

Each entry in the `sections` array becomes a `ConfigSection` in the generated QML.

```json
{
    "title": "Battery",
    "image": "/qmlimages/Battery.svg",
    "showWhen": "_someCondition",
    "controls": [ ... ],
    "repeat": { ... }
}
```

| Field | Purpose |
|---|---|
| `title` | Section heading and sidebar label. Use `{index}` for repeat index substitution. |
| `image` | Icon path for the section header |
| `showWhen` | QML expression — section is hidden when this evaluates to `false` |
| `component` | Escape hatch: name of a hand-written QML component to embed instead of generated controls |
| `controls` | Array of control definitions (see below) |
| `repeat` | Repeat the section for each indexed parameter instance |

### Component Escape Hatch

When a section is too complex for the JSON schema, use `component` to embed hand-written QML:

```json
{
    "title": "Return To Launch",
    "image": "/qmlimages/ReturnToHomeAltitude.svg",
    "component": "FlightModeRTLSettings"
}
```

### Repeat Sections

Repeat sections expand at runtime into one `ConfigSection` per parameter instance (e.g. one per battery).

```json
"repeat": {
    "paramPrefix": "BAT",
    "probePostfix": "_SOURCE",
    "startIndex": 1,
    "firstIndexOmitsNumber": false,
    "enableParam": "_MONITOR",
    "disabledParamValue": "_monitorDisabled",
    "disabledSection": {
        "heading": "Disabled Batteries",
        "enabledParamValue": "_monitorAnalogVoltageAndCurrent"
    }
}
```

| Field | Purpose |
|---|---|
| `paramPrefix` | Base parameter name prefix (e.g. `"BAT"`, `"BATT"`) |
| `probePostfix` | Postfix appended to discover how many instances exist |
| `startIndex` | First index number (usually `1`) |
| `firstIndexOmitsNumber` | When `true`, first instance uses bare prefix (e.g. `BATT` not `BATT1`) |
| `indexing` | Custom indexing mode (e.g. `"apm_battery"` for ArduPilot's non-contiguous naming) |
| `enableParam` | Postfix of the parameter that controls whether an instance is enabled |
| `disabledParamValue` | Constant/binding name — instance is disabled when `enableParam` equals this value |
| `disabledSection` | Optional: adds a companion section listing disabled instances with re-enable buttons |

Inside a repeat section, controls use parameter **postfixes** (e.g. `"_SOURCE"` not `"BAT1_SOURCE"`).
The generator provides a `_fullParamName(postfix)` JS function that constructs the full parameter name at runtime.

## Controls

Each control in a section's `controls` array generates a UI element.

### Common Control Properties

| Property | Purpose |
|---|---|
| `param` | Vehicle parameter name (or postfix inside repeat sections) |
| `label` | Display label. If omitted, derived from Fact metadata. |
| `control` | UI control type (see below). Auto-detected when omitted. |
| `optional` | `true` if the parameter may not exist on some vehicles |
| `showWhen` | QML expression — control is hidden when `false` |
| `enableWhen` | QML expression — control is disabled when `false` |
| `indent` | `true` to indent the control with a left margin |
| `warning` | `true` for label controls to use warning color |

### Control Types

| Type | Description |
|---|---|
| `combobox` | Dropdown bound to Fact enum values |
| `textfield` | Text input bound to Fact value |
| `checkbox` | Checkbox bound to boolean Fact |
| `slider` | Slider with optional `sliderMin`/`sliderMax`, `enableCheckbox`, `button` |
| `radiogroup` | Radio button group with `options` array |
| `label` | Static text label (no parameter binding needed) |
| `dialogButton` | Parameter field with adjacent button that opens a dialog |
| `actionButton` | Standalone button that calls a controller method |
| `bitmask` | Bitmask editor for bitfield parameters |
| `bitmaskCheckbox` | Individual checkbox for a single bit in a bitmask |
| `toggleCheckbox` | Checkbox with custom `checked` and `onClicked` expressions |

### Dialog Button Example

```json
{
    "param": "_V_DIV",
    "label": "Voltage divider",
    "optional": true,
    "showWhen": "controller.parameterExists(-1, _fullParamName(\"_V_DIV\"))",
    "control": "dialogButton",
    "dialogButton": {
        "text": "Calculate",
        "dialogComponent": "CalcVoltageDividerDialog",
        "buttonAfter": false,
        "dialogParams": {
            "batteryIndex": "_rawIndex"
        }
    }
}
```

### Action Button Example

```json
{
    "control": "actionButton",
    "actionButton": {
        "text": "Start Assignment",
        "onClicked": "controller.startBusConfigureActuators()"
    }
}
```

## Base Class: VehicleComponent

`VehicleComponent` (in `src/AutoPilotPlugins/VehicleComponent.h`) provides the C++ side for generated pages.
It reads the same JSON file at runtime to:

- Build the **sidebar section list** (including expanded repeat sections)
- **Dynamically filter** disabled repeat instances from the sidebar
- **Connect signals** so the sidebar updates when enable parameters change

Subclasses only need to implement `vehicleConfigJson()` to return the path to the JSON file and `setupSource()` to return the generated QML URL.

## Adding a New Generated Setup Page

1. Create a new JSON file in the appropriate `VehicleConfig/` directory:

   ```
   src/AutoPilotPlugins/APM/VehicleConfig/MyFeature.VehicleConfig.json
   ```

2. Create a minimal C++ component class inheriting `VehicleComponent`:
   - Implement `vehicleConfigJson()` to return the JSON path
   - Implement `setupSource()` to return the generated QML URL

3. Register the component in the firmware plugin's `AutoPilotPlugin` subclass.

4. Add the generated QML filename to `_config_generated_qml_names` in the plugin's `CMakeLists.txt`.

5. Build. CMake will regenerate the QML automatically:

   ```bash
   cmake --build build --config Debug --target GenerateConfigQmlAPM  # or GenerateConfigQmlPX4
   ```

## Important Notes

- The `constants`, `params`, and `bindings` sections should appear **before** `sections` in the JSON file for readability, since sections reference them.
- Constants should use native JSON numbers, not strings (e.g. `0` not `"0"`).
- The `_config_generated_qml_names` list in CMake is explicit. If you forget to add a new page output file, integration will be incomplete.
- Inside repeat sections, use parameter **postfixes** in `param` fields and `_fullParamName()` in `showWhen` expressions.
- The `enableParam`/`disabledParamValue` pair is required together — you cannot specify one without the other.
