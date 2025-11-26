# AM32 EEPROM Schema Tools

This directory contains tools for working with the AM32 EEPROM schema - a unified
JSON specification that defines the EEPROM layout, settings metadata, and version-specific
behavior for AM32 ESC firmware.

## Schema Overview

The `am32-eeprom-schema.json` file (in the parent directory) is the single source of truth
for AM32 EEPROM settings. It defines:

- **Field offsets** - byte positions in the EEPROM
- **Data types** - bool, uint8, number, enum
- **Min/max ranges** - valid value ranges for each setting
- **Display conversions** - factor/offset to convert raw bytes to display values
- **Units** - measurement units (°, V, A, µs, etc.)
- **Version gating** - which EEPROM/firmware versions support each field
- **Descriptions** - human-readable documentation

## Tools

### generate_am32_header.py

Generates the C `eeprom.h` header file for AM32 firmware.

```bash
python generate_am32_header.py ../am32-eeprom-schema.json > eeprom_generated.h

# Generate offset constants for compile-time validation
python generate_am32_header.py ../am32-eeprom-schema.json --offsets > eeprom_offsets.h
```

### generate_qgc_settings.py

Generates C++ settings configuration for QGroundControl.

```bash
# Generate for EEPROM version 3 (default)
python generate_qgc_settings.py ../am32-eeprom-schema.json > am32_settings.cpp

# Generate for specific EEPROM version
python generate_qgc_settings.py ../am32-eeprom-schema.json --eeprom-version=4

# Generate TypeScript for am32-configurator
python generate_qgc_settings.py ../am32-eeprom-schema.json --typescript > eeprom.ts
```

### validate_eeprom_h.py

Validates an existing `eeprom.h` against the schema to detect mismatches.

```bash
python validate_eeprom_h.py ../am32-eeprom-schema.json /path/to/AM32/Inc/eeprom.h
```

## Display Value Conversion

The schema uses a standard conversion formula:

```
display_value = raw_value * factor + offset
raw_value = (display_value - offset) / factor
```

For example, motor KV:
- Raw range: 0-255
- Factor: 40, Offset: 20
- Display range: 20-10220 KV
- Raw 0 → Display 20 KV
- Raw 255 → Display 10220 KV

## Version Gating

Fields can be gated by EEPROM version or firmware version:

```json
{
  "maxRampSpeed": {
    "minEepromVersion": 3,
    "description": "Only available in EEPROM v3+"
  },
  "autoTiming": {
    "minFirmwareVersion": "2.16",
    "description": "Added in firmware 2.16"
  }
}
```

Some fields have version-specific behavior:

```json
{
  "timingAdvance": {
    "versions": {
      "default": {
        "display": { "factor": 7.5, "offset": 0 }
      },
      "eeprom:3+": {
        "display": { "factor": 1, "offset": -10 }
      }
    }
  }
}
```

## Integration with CI

Add schema validation to your CI pipeline:

```yaml
# .github/workflows/validate-eeprom.yml
- name: Validate EEPROM schema
  run: |
    python tools/am32_schema/validate_eeprom_h.py \
      am32-eeprom-schema.json \
      Inc/eeprom.h
```

## Contributing

When modifying EEPROM layout:

1. Update `am32-eeprom-schema.json` first
2. Run generators to update derived files
3. Validate existing implementations still match
4. Submit changes to all affected repositories

## Projects Using This Schema

- **AM32 Firmware** - Source of truth for EEPROM struct
- **AM32 Configurator** - Web-based ESC configuration
- **QGroundControl** - Ground control station ESC settings
