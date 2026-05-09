# Vehicle Config QML Generator

Generates QML vehicle configuration pages from JSON definitions.

Each `<Name>.VehicleConfig.json` file in a `VehicleConfig/` directory is
converted to `<Name>Component.qml` at CMake configure time.

## Quick start

```bash
python3 tools/generators/config_qml/generate_pages.py \
    --pages-dir src/AutoPilotPlugins/APM/VehicleConfig \
    --output-dir /tmp/generated
```

---

## JSON schema

### Top-level object

| Key | Type | Required | Description |
|-----|------|----------|-------------|
| `fileType` | `"VehicleConfig"` | yes | Must be `"VehicleConfig"` |
| `version` | `1` | yes | Schema version, currently `1` |
| `constants` | object | no | Named literal values reusable in QML expressions |
| `params` | object | no | Named parameter fact declarations (see [params](#params)) |
| `bindings` | object | no | Named QML property bindings available to the page |
| `imports` | array of strings | no | Extra QML `import` lines to emit |
| `controllerType` | string | no | QML type for the controller; default `FactPanelController` |
| `sections` | array of [Section](#section) | yes | Page sections, rendered top-to-bottom |

### `params`

Declares parameter facts the page needs. Each key becomes a QML property; each
value is either a parameter name string or an object:

```json
"params": {
    "rateParam": "LOG_FILE_RATEMAX",
    "backendParam": {
        "name": "LOG_BACKEND_TYPE",
        "required": false,
        "existsOnly": false
    }
}
```

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `name` | string | — | MAVLink parameter name |
| `required` | bool | `false` | `true` → `getParameterFact(-1, name)` (asserts if missing); `false` → `getParameterFact(-1, name, false)` (returns null) |
| `existsOnly` | bool | `false` | `true` → `parameterExists(-1, name)` — exposes a boolean instead of a fact |

Shorthand: `"rateParam": "LOG_FILE_RATEMAX"` is equivalent to `{ "name": "LOG_FILE_RATEMAX", "required": false }`.

---

### `section`

| Key | Type | Required | Description |
|-----|------|----------|-------------|
| `title` | string | yes | Section heading (translatable) |
| `image` | string | no | `qrc:` path for a section icon |
| `showWhen` | string | no | QML expression; section hidden when falsy |
| `keywords` | array of strings | no | Extra search terms for the section filter |
| `controls` | array of [Control](#control) | yes* | Controls to render |
| `component` | string | no | Escape hatch — name of a hand-written QML component to embed instead of generating controls |
| `repeat` | [Repeat](#repeat) | no | Repeat this section for each indexed parameter instance |

\* Not required when `component` is set.

---

### `control`

| Key | Type | Required | Description |
|-----|------|----------|-------------|
| `param` | string | one of `param`/`setting`/`component`/`actionButton` | MAVLink parameter name |
| `setting` | string | — | Settings fact path, e.g. `"flyViewSettings.showAltimeter"` |
| `label` | string | no | Override label; empty → uses `fact.shortDescription` |
| `control` | string | no | Explicit control type (see [Control types](#control-types)); auto-detected if omitted |
| `optional` | bool | `false` | Parameter may not exist. Uses `getParameterFact(-1, name, false)`. Control is hidden (`visible: fact !== null`) when the parameter is absent. |
| `showWhen` | string | no | Extra QML visibility expression. When combined with `optional: true`, the control is only visible when both the parameter exists *and* this expression is truthy. |
| `enableWhen` | string | no | QML expression bound to `enabled` |
| `indent` | bool | `false` | Indent control with `Layout.leftMargin` |
| `warning` | bool | `false` | (label only) Render text in warning color |
| `smallFont` | bool | `false` | (label only) Use smaller font |
| `component` | string | no | Inline escape hatch — hand-written QML component name |

---

### Control types

The `control` key selects the generated widget. When omitted, the generator
auto-detects based on fact metadata (bool → checkbox, enum → combobox, etc.).

| `control` value | Widget | Extra keys |
|-----------------|--------|------------|
| *(omitted)* | Auto-detected | — |
| `combobox` | `LabelledFactComboBox` | `enumValues` for manual value/label list |
| `textfield` | `LabelledFactTextField` | — |
| `checkbox` | `FactCheckBoxSlider` | — |
| `toggleCheckbox` | `QGCCheckBoxSlider` with custom logic | `toggleCheckbox` |
| `slider` | Slider with optional enable-checkbox and adjacent button | `sliderMin`, `sliderMax`, `enableCheckbox`, `button` |
| `factslider` | `FactSlider` in a `SettingsGroupLayout` | `sliderFrom`, `sliderTo`, `majorTickStepSize`, `decimalPlaces`, `description`, `linkedParams` |
| `radiogroup` | Label + radio buttons | `options`, `raw` |
| `bitmask` | `FactBitmask` | `firstEntryIsAll` |
| `bitmaskCheckbox` | `FactBitMaskCheckBoxSlider` | `bitMask` |
| `dialogButton` | Button that opens a popup dialog | `dialogButton` |
| `actionButton` | Standalone button calling a controller method | `actionButton` |
| `label` | Static text label | `warning`, `smallFont` |

#### `combobox` with `enumValues`

When a parameter has no enum metadata (e.g. the firmware doesn't expose it),
supply a manual value/label list:

```json
{
    "param": "EK3_LOG_LEVEL",
    "control": "combobox",
    "optional": true,
    "enumValues": [
        { "value": 0, "label": "Full logging" },
        { "value": 1, "label": "XKF4 scaled innovations only" },
        { "value": 2, "label": "XKF4 and GSF" },
        { "value": 3, "label": "Disabled" }
    ]
}
```

`value` must be a number or string. `label` must be a string.

#### `factslider` keys

| Key | Type | Description |
|-----|------|-------------|
| `sliderFrom` | number | Minimum value override |
| `sliderTo` | number | Maximum value override |
| `majorTickStepSize` | number | Tick interval |
| `decimalPlaces` | number | Decimal precision |
| `description` | string | Help text rendered above the slider |
| `linkedParams` | object | `{ "PARAM_NAME": "expression" }` — params updated `onValueChanged` |

`linkedParams` expression: `value` refers to the slider's current value.

```json
"linkedParams": {
    "ATC_RAT_PIT_P": "value",
    "ATC_RAT_PIT_I": "value * 2"
}
```

#### `slider` keys

| Key | Type | Description |
|-----|------|-------------|
| `sliderMin` | number | Minimum value override |
| `sliderMax` | number | Maximum value override |
| `enableCheckbox` | object | `{ "checked": "expr", "onClicked": "body" }` |
| `button` | object | `{ "text": "label", "onClicked": "body", "enabled": "expr" }` |

#### `radiogroup` keys

| Key | Type | Description |
|-----|------|-------------|
| `options` | array | `[ { "text": "Label", "value": 0 } ]` |
| `raw` | bool | Use `rawValue` instead of `value` for comparisons |

#### `bitmask` keys

| Key | Type | Description |
|-----|------|-------------|
| `firstEntryIsAll` | bool | First checkbox acts as an "all/none" toggle |

#### `bitmaskCheckbox` keys

| Key | Type | Description |
|-----|------|-------------|
| `bitMask` | number | The specific bitmask value this checkbox controls |

---

### `repeat`

Repeats a section for each indexed parameter instance (e.g. `BAT1_…`, `BAT2_…`).

| Key | Type | Description |
|-----|------|-------------|
| `paramPrefix` | string | Parameter prefix, e.g. `"BAT"` |
| `probePostfix` | string | Postfix used to discover instance count, e.g. `"_SOURCE"` |
| `startIndex` | number | First index (default `1`) |
| `firstIndexOmitsNumber` | bool | When `true`, index 1 → `"BAT"` not `"BAT1"` |
| `indexing` | string | Custom indexing mode (e.g. `"apm_battery"`) |
| `enableParam` | string | Param that must differ from `disabledParamValue` for section to show |
| `disabledParamValue` | string | Binding name for the "disabled" value |
| `disabledSection` | object | Companion section shown for disabled instances |

---

## Complete example

```json
{
    "fileType": "VehicleConfig",
    "version": 1,
    "sections": [
        {
            "title": "Storage",
            "keywords": ["logging", "log", "sd card"],
            "controls": [
                {
                    "param": "LOG_BACKEND_TYPE",
                    "label": "Logging backends",
                    "optional": true,
                    "control": "bitmask"
                },
                {
                    "param": "LOG_MAX_FILES",
                    "label": "Maximum retained log files",
                    "optional": true
                }
            ]
        },
        {
            "title": "Options",
            "controls": [
                {
                    "param": "EK3_LOG_LEVEL",
                    "label": "EKF3 logging verbosity",
                    "optional": true,
                    "control": "combobox",
                    "enumValues": [
                        { "value": 0, "label": "Full logging" },
                        { "value": 3, "label": "Disabled" }
                    ]
                }
            ]
        }
    ]
}
```
