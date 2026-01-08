"""
FactMetaData schema definitions for code completion.

This module provides schema information for Fact JSON files to enable
intelligent autocomplete in IDEs.
"""

from dataclasses import dataclass, field
from typing import Optional


@dataclass
class FactProperty:
    """A property in a Fact metadata definition."""
    name: str
    type: str  # "string", "number", "boolean", "array", "enum"
    description: str
    required: bool = False
    enum_values: list[str] = field(default_factory=list)
    default: Optional[str] = None


# Fact metadata properties based on the JSON schema
FACT_PROPERTIES: list[FactProperty] = [
    FactProperty(
        name="name",
        type="string",
        description="Unique identifier for this Fact within the FactGroup",
        required=True,
    ),
    FactProperty(
        name="type",
        type="enum",
        description="Data type of the Fact value",
        required=True,
        enum_values=[
            "uint8", "int8", "uint16", "int16", "uint32", "int32",
            "uint64", "int64", "float", "double", "string", "bool",
            "elapsedTimeInSeconds", "custom"
        ],
    ),
    FactProperty(
        name="units",
        type="string",
        description="Display units for the value (e.g., 'm', 'm/s', 'deg', 'rad', '%')",
    ),
    FactProperty(
        name="min",
        type="number",
        description="Minimum valid value (inclusive)",
    ),
    FactProperty(
        name="max",
        type="number",
        description="Maximum valid value (inclusive)",
    ),
    FactProperty(
        name="default",
        type="any",
        description="Default value when not set",
    ),
    FactProperty(
        name="decimalPlaces",
        type="number",
        description="Number of decimal places for display (0-15)",
    ),
    FactProperty(
        name="shortDesc",
        type="string",
        description="Brief description (shown in UI)",
    ),
    FactProperty(
        name="longDesc",
        type="string",
        description="Detailed description (shown in tooltips/help)",
    ),
    FactProperty(
        name="category",
        type="string",
        description="Category for grouping in UI",
    ),
    FactProperty(
        name="group",
        type="string",
        description="Sub-group within category",
    ),
    FactProperty(
        name="enumStrings",
        type="array",
        description="Display strings for enumerated values",
    ),
    FactProperty(
        name="enumValues",
        type="array",
        description="Numeric values corresponding to enumStrings",
    ),
    FactProperty(
        name="bitmaskStrings",
        type="array",
        description="Display strings for bitmask values",
    ),
    FactProperty(
        name="bitmaskValues",
        type="array",
        description="Numeric values corresponding to bitmaskStrings",
    ),
    FactProperty(
        name="volatileValue",
        type="boolean",
        description="True if value changes frequently (affects UI refresh)",
        default="false",
    ),
    FactProperty(
        name="hasControl",
        type="boolean",
        description="True if this Fact has an associated UI control",
        default="false",
    ),
    FactProperty(
        name="readOnly",
        type="boolean",
        description="True if value cannot be modified by user",
        default="false",
    ),
    FactProperty(
        name="writeOnly",
        type="boolean",
        description="True if value is write-only (e.g., commands)",
        default="false",
    ),
    FactProperty(
        name="increment",
        type="number",
        description="Step increment for UI controls",
    ),
    FactProperty(
        name="nanUnchanged",
        type="boolean",
        description="True if NaN means 'unchanged' (for MAVLink params)",
        default="false",
    ),
    FactProperty(
        name="rebootRequired",
        type="boolean",
        description="True if changing this parameter requires reboot",
        default="false",
    ),
]

# Build lookup dictionary
FACT_PROPERTIES_BY_NAME: dict[str, FactProperty] = {
    prop.name: prop for prop in FACT_PROPERTIES
}

# Common unit values for completion
COMMON_UNITS: list[tuple[str, str]] = [
    ("m", "meters"),
    ("m/s", "meters per second"),
    ("m/s²", "meters per second squared"),
    ("cm", "centimeters"),
    ("cm/s", "centimeters per second"),
    ("mm", "millimeters"),
    ("km", "kilometers"),
    ("deg", "degrees"),
    ("rad", "radians"),
    ("rad/s", "radians per second"),
    ("deg/s", "degrees per second"),
    ("degE7", "degrees × 10⁷ (MAVLink lat/lon)"),
    ("%", "percent"),
    ("s", "seconds"),
    ("ms", "milliseconds"),
    ("us", "microseconds"),
    ("Hz", "Hertz"),
    ("V", "Volts"),
    ("mV", "millivolts"),
    ("A", "Amperes"),
    ("mA", "milliamps"),
    ("mAh", "milliamp-hours"),
    ("W", "Watts"),
    ("°C", "degrees Celsius"),
    ("cdegC", "centidegrees Celsius"),
    ("Pa", "Pascals"),
    ("hPa", "hectopascals"),
    ("mbar", "millibars"),
    ("gauss", "Gauss (magnetic field)"),
    ("mgauss", "milliGauss"),
    ("g", "gravitational acceleration"),
    ("mg", "milli-g"),
    ("norm", "normalized (0-1)"),
]

# Root-level JSON keys
ROOT_KEYS: list[tuple[str, str]] = [
    ("version", "Schema version number (integer)"),
    ("fileType", "File type identifier (usually 'FactMetaData')"),
    ("QGC.MetaData.Facts", "Array of Fact metadata definitions"),
]


def get_type_values() -> list[str]:
    """Get list of valid Fact type values."""
    type_prop = FACT_PROPERTIES_BY_NAME.get("type")
    return type_prop.enum_values if type_prop else []
