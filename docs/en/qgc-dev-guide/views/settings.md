# Settings View

The QGC Application Settings UI is built from a mix of generated and hand-written QML.

- The settings container/sidebar is implemented in [src/QmlControls/AppSettings.qml](../../../../src/QmlControls/AppSettings.qml).
- Most settings content pages are generated from JSON definitions in [src/UI/AppSettings/pages](../../../../src/UI/AppSettings/pages).
- Some pages are still hand-written and referenced directly (for example Help/Logging/debug pages).

## How It Is Generated

The generation pipeline is documented in: [Generated Settings Pages](settings_generation.md)

That page explains:

- Which JSON files define settings pages and controls
- How `*.SettingsGroup.json` Fact metadata drives labels/types/visibility
- How CMake runs the Python generator
- How to add a new setting to an existing page
- How to add an entirely new settings page
