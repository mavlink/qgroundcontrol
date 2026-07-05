# Generated Settings Pages

This page explains how the Application Settings pages are generated from JSON definitions and how to extend them.

For the complete JSON schema reference see [tools/generators/settings_qml/README.md](../../../../tools/generators/settings_qml/README.md).

## Architecture Overview

The runtime stack is:

1. Fact metadata in `src/Settings/*.SettingsGroup.json`
2. Settings Fact accessors in `src/Settings/*Settings.h/.cc`
3. Settings UI page definitions in `src/AppSettings/pages/*.SettingsUI.json`
4. Page list in `src/AppSettings/pages/SettingsPages.json`
5. Python generator in `tools/generators/settings_qml`
6. Generated QML loaded by `src/QmlControls/AppSettings.qml`

At build time, CMake runs the generator and places generated QML in the build tree. Those files are then compiled into the `QGroundControl.AppSettings` QML module.

## Where Generation Is Wired

Generation is configured in [src/AppSettings/CMakeLists.txt](../../../../src/AppSettings/CMakeLists.txt):

- Custom command runs:
  - `python -m tools.generators.settings_qml.generate_pages --output-dir <build>/generated`
- Inputs:
  - `src/AppSettings/pages/*.json`
  - `src/Settings/*.SettingsGroup.json`
- Outputs:
  - Generated page QML files (e.g. `GeneralSettings.qml`, `FlyViewSettings.qml`)
  - `SettingsPagesModel.qml`

The generator entry point is [tools/generators/settings_qml/generate_pages.py](../../../../tools/generators/settings_qml/generate_pages.py), with most logic in [tools/generators/settings_qml/page_generator.py](../../../../tools/generators/settings_qml/page_generator.py).

## How Controls Are Chosen

When `control` is omitted in a `*.SettingsUI.json`, the generator reads the fact's type from `*.SettingsGroup.json` metadata to auto-detect:

- `bool` → checkbox
- Enum-backed facts → combobox
- Other types → text field

An explicit `control` key overrides auto-selection.

## Sidebar, Sections, and Search

The generated `SettingsPagesModel.qml` is built from `SettingsPages.json` and each page definition.

It includes:

- `sections`: section names for expandable sidebar rows
- `searchTerms`: page/section/fact keyword tokens used by the search field in [src/QmlControls/AppSettings.qml](../../../../src/QmlControls/AppSettings.qml)

Search terms are derived from:

- Page name
- Section heading/section name
- Fact metadata `keywords`
- Group-level `keywords` when using `component` groups without explicit controls

## Add a New Setting to an Existing Generated Page

1. Add a Fact metadata entry to the appropriate `src/Settings/<Group>.SettingsGroup.json` file.
2. Expose that Fact through the corresponding settings class:
   - Add `DEFINE_SETTINGFACT(<factName>)` in the matching `*Settings.h`.
   - Ensure `DECLARE_SETTINGSFACT` exists in `*Settings.cc` if required by that file pattern.
3. Add a control entry in the page JSON:

   - File: `src/AppSettings/pages/<Page>.SettingsUI.json`
   - Add: `{ "setting": "<accessor>.<factName>" }`

   See [tools/generators/settings_qml/README.md](../../../../tools/generators/settings_qml/README.md) for the full JSON schema.
4. Build. CMake regenerates the QML page automatically.

## Add a New Generated Settings Page

1. Create a new page definition JSON in `src/AppSettings/pages`, e.g. `MyFeature.SettingsUI.json`.
2. Add a new entry to `src/AppSettings/pages/SettingsPages.json`:
   - `name`, `icon`, `qml` (output filename), `pageDefinition` (your new JSON file)
   - Optional `visible` expression
3. Update the generated outputs list in [src/AppSettings/CMakeLists.txt](../../../../src/AppSettings/CMakeLists.txt):
   - Add your new QML filename to `_generated_qml_names`.
4. Build QGC to generate and include the new page.

## Important Notes

- If a page in `SettingsPages.json` has no `pageDefinition`, it is treated as hand-written QML/URL content and not generated.
- The CMake `_generated_qml_names` list is explicit. If you forget to add a new output filename, build integration will be incomplete.
- The `setting` path in `*.SettingsUI.json` must match a valid `QGroundControl.settingsManager.<group>.<fact>` accessor.
- Fact labels should be present in metadata. Missing labels are logged at runtime by `SettingsGroup`.
