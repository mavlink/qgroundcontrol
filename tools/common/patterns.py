"""
QGC-specific regex patterns for code analysis.

These patterns are used by multiple tools (analyzer, locator, etc.)
for consistent detection of QGC code constructs.
"""

import re

# =============================================================================
# Fact System Patterns
# =============================================================================

# Match: Fact _speedFact = Fact(0, ...)
# Captures: fact name (without Fact suffix)
FACT_MEMBER_PATTERN = re.compile(
    r'Fact\s+_(\w+)Fact\s*='
)

# Match: class VehicleGPSFactGroup : public FactGroup
# Captures: full class name
FACTGROUP_CLASS_PATTERN = re.compile(
    r'class\s+(\w+FactGroup)\s*:'
)

# Match: "name": "latitude" in JSON
# Captures: parameter name
PARAM_NAME_PATTERN = re.compile(
    r'"name"\s*:\s*"(\w+)"'
)


# =============================================================================
# MAVLink Patterns
# =============================================================================

# Match: MAVLINK_MSG_ID_HEARTBEAT, MAVLINK_MSG_ID_GPS_RAW_INT
# Captures: message name (without MAVLINK_MSG_ID_ prefix)
MAVLINK_MSG_ID_PATTERN = re.compile(
    r'MAVLINK_MSG_ID_(\w+)'
)

# Match: mavlink_heartbeat_t, mavlink_gps_raw_int_t
# Captures: message name (lowercase)
MAVLINK_STRUCT_PATTERN = re.compile(
    r'mavlink_(\w+)_t'
)


# =============================================================================
# Null Safety Patterns (for analyzer)
# =============================================================================

# Match: activeVehicle()->method()
# Captures: method name
ACTIVE_VEHICLE_DIRECT_PATTERN = re.compile(
    r'activeVehicle\(\)\s*->\s*(\w+)\s*\('
)

# Match: vehicle = activeVehicle() or vehicle = MultiVehicleManager::instance()->activeVehicle()
# Captures: variable name
ACTIVE_VEHICLE_ASSIGN_PATTERN = re.compile(
    r'(\w+)\s*=\s*(?:MultiVehicleManager::instance\(\)->)?activeVehicle\(\)'
)

# Match: getParameter(...)->method()
# Captures: method name
GET_PARAMETER_DIRECT_PATTERN = re.compile(
    r'getParameter\s*\([^)]*\)\s*->\s*(\w+)\s*\('
)

# Patterns that indicate a null check has been performed
NULL_CHECK_PATTERNS = [
    r'if\s*\(\s*!\s*\w+\s*\)',            # if (!var)
    r'if\s*\(\s*\w+\s*==\s*nullptr\s*\)',  # if (var == nullptr)
    r'if\s*\(\s*\w+\s*!=\s*nullptr\s*\)',  # if (var != nullptr)
    r'if\s*\(\s*\w+\s*\)',                 # if (var)
    r'\?\s*:',                             # ternary operator
]


# =============================================================================
# Query Helpers
# =============================================================================

def make_query_pattern(base_pattern: re.Pattern, query: str) -> re.Pattern:
    """
    Create a pattern that matches base_pattern filtered by query.

    Args:
        base_pattern: The base regex pattern with a capture group
        query: User query to filter results (case-insensitive)

    Returns:
        New pattern that only matches if capture group contains query
    """
    # Insert query filter into the capture group
    pattern_str = base_pattern.pattern
    # Replace \w+ or \w* in first capture group with query-filtered version
    filtered = pattern_str.replace(
        r'(\w+)',
        rf'(\w*{re.escape(query)}\w*)',
        1  # Only first occurrence
    )
    return re.compile(filtered, re.IGNORECASE)
