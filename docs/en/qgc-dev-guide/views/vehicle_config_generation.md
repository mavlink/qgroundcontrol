# Generated Vehicle Config Pages

This page explains how the Vehicle Setup pages (Power, Safety, etc.) are generated from JSON definitions and how to extend them.

For the complete JSON schema reference see [tools/generators/config_qml/README.md](https://github.com/mavlink/qgroundcontrol/blob/master/tools/generators/config_qml/README.md).

## Architecture Overview

The generation pipeline is:

1. **JSON page definitions** in `src/AutoPilotPlugins/APM/VehicleConfig/*.VehicleConfig.json`
2. **Python generator** in `tools/generators/config_qml/` (`model.py` loads the JSON, `emit.py` renders QML)
3. **Generated QML** in `build/src/AutoPilotPlugins/APM/generated/`
4. **C++ component class** (e.g. `APMPowerComponent`) loads the generated QML via `setupSource()`

At build time, CMake runs the generator for the ArduPilot plugin and places generated QML files in the build tree.
Those files are compiled into the `QGroundControl.AutoPilotPlugins.APM` QML module.

## Where Generation Is Wired

Generation is configured in the ArduPilot plugin's `CMakeLists.txt`:

- `src/AutoPilotPlugins/APM/CMakeLists.txt` — target `GenerateConfigQmlAPM`

PX4 setup pages are hand-written QML built from row definitions, so PX4 has no generation target.

Each runs:

```bash
python -m tools.generators.config_qml.generate_pages \
    --pages-dir <pages_dir> \
    --output-dir <build_gen_dir>
```

The generator entry point is `tools/generators/config_qml/generate_pages.py`; `model.py` loads and validates page definitions and `emit.py` renders the QML.

## Base Class: VehicleComponent

`VehicleComponent` (in `src/AutoPilotPlugins/VehicleComponent.h`) provides the C++ side for generated pages.
It reads the same JSON file at runtime to:

- Build the **sidebar section list** (including expanded repeat sections)
- **Dynamically filter** disabled repeat instances from the sidebar
- **Connect signals** so the sidebar updates when enable parameters change

Subclasses only need to implement `vehicleConfigJson()` to return the path to the JSON file and `setupSource()` to return the generated QML URL.

## Adding a New Generated Setup Page

1. Create a new JSON file in the appropriate `VehicleConfig/` directory:

   ```text
   src/AutoPilotPlugins/APM/VehicleConfig/MyFeature.VehicleConfig.json
   ```

   See [tools/generators/config_qml/README.md](https://github.com/mavlink/qgroundcontrol/blob/master/tools/generators/config_qml/README.md) for the full JSON schema.

2. Create a minimal C++ component class inheriting `VehicleComponent`:
   - Implement `vehicleConfigJson()` to return the JSON path
   - Implement `setupSource()` to return the generated QML URL

3. Register the component in the firmware plugin's `AutoPilotPlugin` subclass.

4. Build. CMake picks up the new JSON file and generates its QML automatically:

   ```bash
   cmake --build build --config Debug --target GenerateConfigQmlAPM
   ```

## Important Notes

- The `constants`, `params`, and `bindings` sections should appear **before** `sections` in the JSON file for readability, since sections reference them.
- Constants should use native JSON numbers, not strings (e.g. `0` not `"0"`).
- Inside repeat sections, use parameter **postfixes** in `param` fields and `_fullParamName()` in `showWhen` expressions.
- The `enableParam`/`disabledParamValue` pair is required together — you cannot specify one without the other.
