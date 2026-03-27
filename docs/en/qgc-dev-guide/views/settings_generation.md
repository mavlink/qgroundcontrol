# Generated Settings Pages

This page explains how the new Application Settings pages are generated and how to extend them.

## Architecture Overview

The runtime stack is:

1. Fact metadata in `src/Settings/*.SettingsGroup.json`
2. Settings Fact accessors in `src/Settings/*Settings.h/.cc`
3. Settings UI page definitions in `src/UI/AppSettings/pages/*.SettingsUI.json`
4. Page list in `src/UI/AppSettings/pages/SettingsPages.json`
5. Python generator in `tools/generators/settings_qml`
6. Generated QML loaded by `src/QmlControls/AppSettings.qml`

At build time, CMake runs the generator and places generated QML in the build tree. Those files are then compiled into the `QGroundControl.AppSettings` QML module.

## Where Generation Is Wired

Generation is configured in [src/UI/AppSettings/CMakeLists.txt](../../../../src/UI/AppSettings/CMakeLists.txt):

- Custom command runs:
  - `python -m tools.generators.settings_qml.generate_pages --output-dir <build>/generated`
- Inputs:
  - `src/UI/AppSettings/pages/*.json`
  - `src/Settings/*.SettingsGroup.json`
- Outputs:
  - Generated page QML files (for example `AppSettings.qml`, `FlyViewSettings.qml`, ...)
  - `SettingsPagesModel.qml`

The generator entry point is [tools/generators/settings_qml/generate_pages.py](../../../../tools/generators/settings_qml/generate_pages.py), with most logic in [tools/generators/settings_qml/page_generator.py](../../../../tools/generators/settings_qml/page_generator.py).

## JSON Files and Their Roles

1. `SettingsPages.json` controls sidebar/page registration:
- Page name
- Icon
- Generated QML file name (`qml`)
- Source page definition JSON (`pageDefinition`)
- Optional visibility expression (`visible`)
- Optional divider entries

2. `<Page>.SettingsUI.json` controls page content:
- `bindings`: reusable QML expressions
- `groups`: section blocks
- Group options:
  - `heading`
  - `sectionName` (sidebar label override)
  - `showWhen`, `enableWhen`
  - `component` (embed a custom QML component)
  - `keywords` (for search on component-only groups)
- Control options:
  - `setting` (required), format: `<settingsManager accessor>.<factName>`
  - Optional `control` override (`checkbox`, `combobox`, `textfield`, `slider`, `browse`, `scaler`)
  - Optional `label`, `showWhen`, `enableWhen`, `placeholder`
  - Slider-specific `enableCheckbox` and `button`

3. `*.SettingsGroup.json` provides Fact metadata used by both runtime and generation:
- Type
- Label/descriptions
- Enum values
- Visibility/user visibility
- Search `keywords`

## How Controls Are Chosen

When `control` is omitted, the generator auto-selects from Fact metadata:

- `bool` -> checkbox control
- Enum-backed facts -> combobox
- Other types -> text field

Explicit `control` in `*.SettingsUI.json` overrides auto-selection.

## Sidebar, Sections, and Search

Generated `SettingsPagesModel.qml` is built from `SettingsPages.json` and each page definition.

It includes:

- `sections`: section names for expandable sidebar rows
- `searchTerms`: page/section/fact keyword tokens used by the search field in [src/QmlControls/AppSettings.qml](../../../../src/QmlControls/AppSettings.qml)

Search terms are derived from:

- Page name
- Section heading/section name
- Fact metadata `keywords`
- Group-level `keywords` when using `component` groups without explicit controls

## Add a New Setting to an Existing Generated Page

1. Add Fact metadata entry to the appropriate `src/Settings/<Group>.SettingsGroup.json` file.
2. Expose that Fact through the corresponding settings class:
- Add `DEFINE_SETTINGFACT(<factName>)` in the matching `*Settings.h`.
- Ensure `DECLARE_SETTINGSFACT` exists in `*Settings.cc` if required by that file pattern.
3. Add a control entry in the page JSON:
- File: `src/UI/AppSettings/pages/<Page>.SettingsUI.json`
- Add control: `{ "setting": "<accessor>.<factName>" }`
4. Build QGC. CMake regenerates the QML page automatically.

## Add a New Generated Settings Page

1. Create a new page definition JSON in `src/UI/AppSettings/pages`, for example `MyFeature.SettingsUI.json`.
2. Add a new entry to `src/UI/AppSettings/pages/SettingsPages.json`:
- `name`
- `icon`
- `qml` (output file name, for example `MyFeatureSettings.qml`)
- `pageDefinition` (your new JSON file)
- Optional `visible` expression
3. Update generated outputs list in [src/UI/AppSettings/CMakeLists.txt](../../../../src/UI/AppSettings/CMakeLists.txt):
- Add your new QML file name to `_generated_qml_names`.
4. Build QGC to generate and include the new page.

## Important Notes and Gotchas

- If a page exists in `SettingsPages.json` but has no `pageDefinition`, it is not generated; it is treated as hand-written QML/URL content.
- The CMake `_generated_qml_names` list is explicit. If you forget to add a new page output file name there, generation/build integration will be incomplete.
- The `setting` path in `*.SettingsUI.json` must match a valid `QGroundControl.settingsManager.<group>.<fact>` accessor.
- Fact labels should be present in metadata. Missing labels are logged at runtime by `SettingsGroup`.

## Runtime Override Support

`SettingsManager` can load external settings override files (from `settingsSavePath()` with the configured settings file extension), then apply metadata/value/visibility overrides in `SettingsManager::adjustSettingMetaData(...)`.

This is separate from page generation, but it affects visibility/defaults in generated UI.
