# Generated Vehicle Config Pages

This page explains how the Vehicle Setup pages (Power, Safety, etc.) are generated from JSON definitions and how to extend them.

For the complete JSON schema reference see [tools/generators/config_qml/README.md](../../../../tools/generators/config_qml/README.md).

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

   See [tools/generators/config_qml/README.md](../../../../tools/generators/config_qml/README.md) for the full JSON schema.

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
