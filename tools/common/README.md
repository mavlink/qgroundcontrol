# QGC Tools Common Library

Shared utilities used by QGC developer tools.

## Modules

### patterns.py

QGC-specific regex patterns for code analysis.

```python
from tools.common.patterns import (
    FACT_MEMBER_PATTERN,        # Match: Fact _speedFact = ...
    FACTGROUP_CLASS_PATTERN,    # Match: class VehicleGPSFactGroup : ...
    MAVLINK_MSG_ID_PATTERN,     # Match: MAVLINK_MSG_ID_HEARTBEAT
    PARAM_NAME_PATTERN,         # Match: "name": "latitude" in JSON
    ACTIVE_VEHICLE_DIRECT_PATTERN,   # Match: activeVehicle()->method()
    ACTIVE_VEHICLE_ASSIGN_PATTERN,   # Match: var = activeVehicle()
    GET_PARAMETER_DIRECT_PATTERN,    # Match: getParameter()->method()
    NULL_CHECK_PATTERNS,        # List of null-check pattern strings
    make_query_pattern,         # Create filtered pattern from query
)
```

### file_traversal.py

File discovery with proper filtering for QGC codebase.

```python
from tools.common.file_traversal import (
    find_repo_root,      # Find .git root directory
    find_cpp_files,      # Find .cc, .cpp, .h, .hpp files
    find_header_files,   # Find .h, .hpp files only
    find_source_files,   # Find .cc, .cpp files only
    find_json_files,     # Find JSON files by pattern
    should_skip_path,    # Check if path should be skipped
    DEFAULT_SKIP_DIRS,   # {'build', 'libs', 'test', ...}
)
```

## Usage Example

```python
import sys
from pathlib import Path

# Add tools to path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from common.patterns import FACT_MEMBER_PATTERN, make_query_pattern
from common.file_traversal import find_header_files, find_repo_root

# Search for Facts matching a query
repo_root = find_repo_root()
query_pattern = make_query_pattern(FACT_MEMBER_PATTERN, "lat")

for header in find_header_files(repo_root / "src"):
    content = header.read_text()
    for match in query_pattern.finditer(content):
        print(f"Found: {match.group(1)} in {header}")
```

## Adding New Patterns

1. Add pattern to `patterns.py` with descriptive name and docstring
2. Export from `__init__.py`
3. Add tests if pattern is complex
