# FactGroup Generator

Generate complete FactGroup boilerplate from a YAML or JSON specification.

## Quick Start

```bash
# Install dependencies
pip install jinja2 pyyaml

# Preview generated files
python3 -m tools.generators.factgroup.cli \
  --spec tools/generators/factgroup/examples/wind.yaml \
  --dry-run

# Generate files
python3 -m tools.generators.factgroup.cli \
  --spec tools/generators/factgroup/examples/wind.yaml \
  --output src/Vehicle/FactGroups/
```

## Specification Format

### Basic YAML Spec

```yaml
domain: Wind                    # Required: becomes VehicleWindFactGroup
update_rate_ms: 1000           # Optional: default 1000

facts:
  - name: direction            # Required: camelCase
    type: double               # Required: double, float, uint32, etc.
    units: deg                 # Optional: display units
    short_desc: Wind direction # Optional: UI description
    decimal_places: 1          # Optional: default 2
    min: 0                     # Optional: minimum value
    max: 360                   # Optional: maximum value

mavlink_messages:              # Optional: generates handler stubs
  - WIND_COV                   # Simple: just message ID
```

### Advanced: MAVLink Field Mappings

Specify exactly how MAVLink fields map to Facts with optional scaling:

```yaml
mavlink_messages:
  # Simple - generates TODO stub
  - WIND_COV

  # With field mappings and scaling
  - id: GPS_RAW_INT
    mappings:
      - fact: lat              # Fact to update
        field: lat             # MAVLink field (default: same as fact)
        scaling: "* 1e-7"      # Scaling expression (degE7 → deg)
      - fact: alt
        field: alt
        scaling: "/ 1000.0"    # mm → m
      - fact: groundSpeed
        field: vel
        scaling: "/ 100.0"     # cm/s → m/s

  # ArduPilot dialect (generates #ifndef guard)
  - id: WIND
    dialect: ardupilot
    mappings:
      - fact: direction
        field: direction
      - fact: speed
        field: speed
```

### Transform Expressions

For complex transformations beyond simple scaling:

```yaml
mappings:
  - fact: direction
    field: wind_x
    transform: "qRadiansToDegrees(qAtan2({var}.wind_y, {field}))"
    # {var} = decoded struct name, {field} = struct.source_field
```

### JSON Alternative

```json
{
  "domain": "Wind",
  "update_rate_ms": 1000,
  "facts": [
    {"name": "direction", "type": "double", "units": "deg"}
  ],
  "mavlink_messages": [
    {"id": "GPS_RAW_INT", "mappings": [
      {"fact": "lat", "field": "lat", "scaling": "* 1e-7"}
    ]}
  ]
}
```

## CLI Options

| Option | Short | Description |
|--------|-------|-------------|
| `--spec FILE` | `-s` | Load spec from YAML/JSON file |
| `--name NAME` | `-n` | Domain name (alternative to --spec) |
| `--facts SPECS` | `-f` | Inline facts: "name:type:units,..." |
| `--mavlink MSGS` | `-m` | MAVLink messages: "MSG1,MSG2,..." |
| `--output DIR` | `-o` | Output directory (default: .) |
| `--update-rate MS` | `-r` | Update rate in ms (default: 1000) |
| `--dry-run` | `-d` | Preview without writing files |
| `--validate` | | Only validate spec |

## Generated Files

For `--name Wind`:

| File | Contents |
|------|----------|
| `VehicleWindFactGroup.h` | Class declaration, Q_PROPERTY, accessors |
| `VehicleWindFactGroup.cc` | Constructor, MAVLink handlers with mappings |
| `WindFact.json` | FactMetaData for each fact |

## Common MAVLink Scaling Factors

| Field Pattern | Scaling | Description |
|---------------|---------|-------------|
| lat, lon (degE7) | `* 1e-7` | Degrees × 10^7 → degrees |
| alt (mm) | `/ 1000.0` | Millimeters → meters |
| vel (cm/s) | `/ 100.0` | Centimeters/sec → m/s |
| eph, epv (cm) | `/ 100.0` | Centimeters → meters |
| cog, hdg (cdeg) | `/ 100.0` | Centidegrees → degrees |
| temperature (cdegC) | `/ 100.0` | Centidegrees → °C |
| press_abs (hPa→Pa) | `* 100.0` | Hectopascals → Pascals |

## Supported Types

| Type | C++ Type | FactMetaData |
|------|----------|--------------|
| `double` | `double` | `valueTypeDouble` |
| `float` | `float` | `valueTypeFloat` |
| `uint8` | `uint8_t` | `valueTypeUint8` |
| `uint16` | `uint16_t` | `valueTypeUint16` |
| `uint32` | `uint32_t` | `valueTypeUint32` |
| `uint64` | `uint64_t` | `valueTypeUint64` |
| `int8` | `int8_t` | `valueTypeInt8` |
| `int16` | `int16_t` | `valueTypeInt16` |
| `int32` | `int32_t` | `valueTypeInt32` |
| `int64` | `int64_t` | `valueTypeInt64` |
| `string` | `QString` | `valueTypeString` |
| `bool` | `bool` | `valueTypeBool` |

## After Generation

1. Add files to `src/Vehicle/FactGroups/CMakeLists.txt`
2. Add `#include "VehicleWindFactGroup.h"` to `Vehicle.h`
3. Add member: `VehicleWindFactGroup _windFactGroup;`
4. Initialize in `Vehicle.cc` constructor
5. Call `_addFactGroup(&_windFactGroup, "wind")` in `_commonInit()`
6. Review generated MAVLink handlers and adjust as needed

## Examples

See `examples/` directory:
- `wind.yaml` - Wind with scaling, ArduPilot dialect
- `gps.yaml` - GPS with common degE7/mm scaling patterns

## Customizing Templates

Templates are in `templates/`:
- `header.h.j2` - Header file template
- `source.cc.j2` - Source file template
- `metadata.json.j2` - JSON metadata template

Modify these to change the generated code style.
