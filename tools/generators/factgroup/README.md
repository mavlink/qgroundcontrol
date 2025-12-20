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

### YAML (Recommended)

```yaml
# wind.yaml
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

  - name: speed
    type: double
    units: m/s

mavlink_messages:              # Optional: generates handler stubs
  - WIND_COV
  - HIGH_LATENCY2
```

### JSON Alternative

```json
{
  "domain": "Wind",
  "update_rate_ms": 1000,
  "facts": [
    {"name": "direction", "type": "double", "units": "deg"}
  ],
  "mavlink_messages": ["WIND_COV"]
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
| `VehicleWindFactGroup.cc` | Constructor, MAVLink handlers |
| `WindFact.json` | FactMetaData for each fact |

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
6. Fill in MAVLink handler TODOs with actual field mappings

## Examples

See `examples/` directory for sample specifications.

## Customizing Templates

Templates are in `templates/`:
- `header.h.j2` - Header file template
- `source.cc.j2` - Source file template
- `metadata.json.j2` - JSON metadata template

Modify these to change the generated code style.
