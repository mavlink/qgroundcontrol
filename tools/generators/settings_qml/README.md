# Settings QML Generator

Generates QML application settings pages from UI definition JSON files.

The generator reads two types of inputs:

- `src/AppSettings/pages/SettingsPages.json` — the ordered list of pages
  shown in the Settings UI
- `src/AppSettings/pages/<Page>.SettingsUI.json` — the layout definition
  for each page

It also reads `src/Settings/*.SettingsGroup.json` to look up fact type metadata
(bool, enum, etc.) so it can auto-detect the right control type.

## Quick start

```bash
python3 -m tools.generators.settings_qml.generate_pages \
    --output-dir src/AppSettings
```

---

## `SettingsPages.json`

Defines the ordered list of pages in the Settings sidebar.

```json
{
    "version": 1,
    "fileType": "SettingsPages",
    "pages": [
        {
            "name": "General",
            "qml": "GeneralSettings.qml",
            "icon": "qrc:/res/QGCLogoWhite.svg",
            "pageDefinition": "General.SettingsUI.json"
        },
        { "divider": true },
        {
            "name": "Comm Links",
            "qml": "CommLinksSettings.qml",
            "icon": "qrc:/InstrumentValueIcons/usb.svg",
            "url": "qrc:/path/to/HandWritten.qml"
        }
    ]
}
```

### Page entry keys

| Key | Type | Description |
| --- | --- | --- |
| `name` | string | Display name in the sidebar |
| `icon` | string | `qrc:` path for the sidebar icon |
| `qml` | string | Output filename (e.g. `GeneralSettings.qml`) |
| `pageDefinition` | string | `*.SettingsUI.json` file to generate from |
| `url` | string | `qrc:` URL of a hand-written QML page (bypasses generation) |
| `visible` | string | QML expression; entry hidden when falsy |
| `divider` | bool | Insert a visual divider instead of a page entry |

A page entry must have exactly one of `pageDefinition` (generated) or `url` (hand-written).

---

## `*.SettingsUI.json`

Defines the layout of a single settings page.

### Top-level object

| Key | Type | Required | Description |
| --- | --- | --- | --- |
| `fileType` | `"SettingsUI"` | yes | Must be `"SettingsUI"` |
| `version` | `1` | yes | Schema version |
| `bindings` | object | no | Named QML property bindings (e.g. accessor aliases) |
| `groups` | array of [Group](#group) | yes | Setting groups displayed on the page |

`bindings` is the standard way to alias a long settings accessor:

```json
"bindings": {
    "_appSettings": "QGroundControl.settingsManager.appSettings"
}
```

The binding name is then available as a QML property anywhere on the page.

---

### `group`

A collapsible group with an optional heading.

| Key | Type | Required | Description |
| --- | --- | --- | --- |
| `heading` | string | no | Group heading (translatable) |
| `headingDescription` | string | no | QML expression for a dynamic description under the heading |
| `showWhen` | string | no | QML expression; group hidden when falsy |
| `enableWhen` | string | no | QML expression; group disabled when falsy |
| `sectionName` | string | no | Tree-nav display name; falls back to `heading` |
| `keywords` | array of strings | no | Extra search terms |
| `component` | string | no | Name of a hand-written QML component to embed instead of generating controls |
| `missing` | array of strings | no | Descriptions of complex UI not yet generated (documentation only) |
| `controls` | array of [Control](#control) | yes* | Controls in this group |

\* Not required when `component` is set.

---

### `control`

| Key | Type | Required | Description |
| --- | --- | --- | --- |
| `setting` | string | yes | Dotted path to the fact, e.g. `"appSettings.qLocaleLanguage"` |
| `label` | string | no | Override label; empty → uses `fact.label` |
| `control` | string | no | Explicit control type (see [Control types](#control-types)); auto-detected if omitted |
| `showWhen` | string | no | Extra QML visibility expression (combined with `fact.userVisible` via logical AND) |
| `enableWhen` | string | no | QML expression bound to `enabled` |
| `placeholder` | string | no | Placeholder text for text fields |
| `enableCheckbox` | object | no | Enable-checkbox for sliders (see below) |
| `button` | object | no | Adjacent button (see below) |

The `setting` value must be `"<settingsGroupAccessor>.<factName>"`, where the
accessor matches the C++ `SettingsManager` property
(e.g. `appSettings`, `flyViewSettings`, `autoConnectSettings`).

---

### Control types

When `control` is omitted, the generator reads the fact's type from
`*.SettingsGroup.json` metadata to auto-detect:

| Fact type | Default control |
| --- | --- |
| `bool` | `checkbox` |
| Has `enumStrings` | `combobox` |
| Numeric | `textfield` |

Explicit `control` values:

| Value | Widget |
| --- | --- |
| `combobox` | `LabelledFactComboBox` |
| `textfield` | `LabelledFactTextField` |
| `checkbox` | `FactCheckBoxSlider` |
| `slider` | Slider with optional enable-checkbox and adjacent button |
| `browse` | File/path browser (desktop only; pair with `showWhen: "!ScreenTools.isMobile"`) |
| `scaler` | Percentage scaler (for `uiScalePercent`) |

#### `slider` extra keys

| Key | Type | Description |
| --- | --- | --- |
| `enableCheckbox` | object | `{ "checked": "expr", "onClicked": "body" }` |
| `button` | object | `{ "text": "label", "onClicked": "body", "enabled": "expr" }` |

---

### objectName conventions (UI test hooks)

Generated pages emit stable `objectName`s so QML UI tests (`test/QmlUITests/`,
via `QmlUITestBase::findVisibleItem`) can locate items without brittle
text/traversal matching:

| Item | objectName |
| --- | --- |
| Page root | `settingsPage_<PageName>` (characters outside `[A-Za-z0-9_]` stripped, e.g. `settingsPage_RemoteID`) |
| Group (`SettingsGroupLayout`, headed groups only) | `settingsGroup_<Heading>` (characters outside `[A-Za-z0-9_]` stripped, e.g. `settingsGroup_EUVehicleInfo`) |
| Text field (`LabelledFactTextField`) | `settingsTextField_<factName>` |
| Checkbox (`FactCheckBoxSlider`) | `settingsCheckBox_<factName>` |

Page names and headings are sanitized to `[A-Za-z0-9_]` before being embedded in
the objectName. Generation fails with an error if a heading sanitizes to an empty
string, or if two headings on the same page collapse to the same objectName —
rename one of the headings to resolve it.

The hand-written `SettingsPage.qml` content flickable is named
`settingsPageFlickable` for use with `QmlUITestBase::scrollIntoView`.
See `test/QmlUITests/RemoteIDSettingsUITest.cc` for a usage example.

---

## Complete example

```json
{
    "version": 1,
    "fileType": "SettingsUI",
    "bindings": {
        "_appSettings": "QGroundControl.settingsManager.appSettings"
    },
    "groups": [
        {
            "heading": "General",
            "keywords": ["language", "locale", "theme", "color scheme"],
            "controls": [
                {
                    "setting": "appSettings.qLocaleLanguage",
                    "control": "combobox"
                },
                {
                    "setting": "appSettings.indoorPalette"
                },
                {
                    "setting": "appSettings.audioVolume",
                    "control": "slider",
                    "enableCheckbox": {
                        "checked": "!_appSettings.audioMuted.rawValue",
                        "onClicked": "{ _appSettings.audioMuted.rawValue = !enableCheckBoxChecked }"
                    },
                    "button": {
                        "text": "Test",
                        "onClicked": "QGroundControl.testAudioOutput()",
                        "enabled": "!_appSettings.audioMuted.rawValue"
                    }
                },
                {
                    "setting": "appSettings.savePath",
                    "control": "browse",
                    "showWhen": "!ScreenTools.isMobile"
                }
            ]
        },
        {
            "heading": "Advanced",
            "component": "MyHandWrittenAdvancedGroup"
        }
    ]
}
```
