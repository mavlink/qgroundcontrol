# QGroundControl Tools Tests

Unit tests for Python development tools and utilities used in QGroundControl builds and analysis.

## Overview

This directory contains pytest test suites for:
- Common utility patterns and regex matching
- FactGroup code generator
- Vehicle null pointer checks analyzer
- Build configuration readers

## Quick Start

```bash
# Run all tests
pytest tools/tests/

# Run with verbose output
pytest -v tools/tests/

# Run specific test file
pytest tools/tests/test_common.py

# Run single test class
pytest tools/tests/test_vehicle_null_check.py::TestNullCheckAnalyzer

# Run with coverage
pytest --cov=tools/analyzers --cov=tools/generators tools/tests/
```

## Test Files

| File | Module | Coverage |
|------|--------|----------|
| **test_common.py** | `tools/common/` | Regex patterns, file traversal, configuration |
| **test_factgroup_generator.py** | `tools/generators/factgroup/` | FactGroup C++ code generation |
| **test_vehicle_null_check.py** | `tools/analyzers/` | Null pointer access detection |

## test_common.py

Tests for shared utilities and patterns used across tools.

**Classes Tested:**

- `Patterns` - Regex patterns for code parsing
  - Fact member declarations (`_speedFact`)
  - FactGroup class definitions
  - MAVLink message IDs
  - Parameter names
  - Vehicle fact access patterns
  - Dynamic pattern generation

- `FileTraversal` - Repository navigation
  - Finding repository root
  - Path filtering (skip node_modules, build/, etc.)
  - Default skip directories

**Example Tests:**

```python
def test_fact_member_pattern():
    """Should match Fact member declarations."""
    line = "    Fact _speedFact = Fact(0, ...);"
    match = FACT_MEMBER_PATTERN.search(line)
    assert match is not None
    assert match.group(1) == "speed"

def test_factgroup_class_pattern():
    """Should match FactGroup class declarations."""
    line = "class VehicleGPSFactGroup : public FactGroup"
    match = FACTGROUP_CLASS_PATTERN.search(line)
    assert match is not None
```

**Running:**

```bash
pytest tools/tests/test_common.py -v
pytest tools/tests/test_common.py::TestPatterns::test_fact_member_pattern
```

## test_factgroup_generator.py

Tests for the FactGroup C++ code generator that creates boilerplate code from YAML specifications.

**Scenarios Tested:**

- **Parser** - Reading and validating YAML specs
  - Valid specifications
  - Invalid YAML format
  - Missing required fields
  - Duplicate member names

- **Generator** - Creating C++ header and implementation files
  - Header file generation
  - Implementation file generation
  - Include guards and headers
  - Getter methods
  - Setter methods
  - Qt signal emissions

- **CLI** - Command-line interface
  - Input file paths
  - Output directory selection
  - Namespace generation

**Fixtures:**

- `test_spec.yaml` - Valid FactGroup specification
- `test_spec_invalid.yaml` - Invalid YAML (for error handling)

**Example Test:**

```python
def test_generates_valid_header(tmpdir):
    """Should generate valid C++ header with getters."""
    spec = load_yaml_spec("test_spec.yaml")
    generator = FactGroupGenerator(spec)

    header = generator.generate_header()

    assert "#pragma once" in header
    assert "class TestFactGroup : public FactGroup" in header
    assert "Q_PROPERTY(int speed READ speed)" in header
```

**Running:**

```bash
pytest tools/tests/test_factgroup_generator.py -v
pytest tools/tests/test_factgroup_generator.py::TestGenerator::test_generates_valid_header
```

## test_vehicle_null_check.py

Tests for the vehicle null pointer check analyzer that detects missing null safety checks.

**Features Tested:**

- **Pattern Matching**
  - Detects `vehicle->` dereferences without null checks
  - Identifies `activeVehicle()` calls with null access
  - Handles nested conditionals

- **File Analysis**
  - Processes C++ source files
  - Tracks control flow context
  - Reports line numbers and code snippets

- **Filtering**
  - Skip test files
  - Skip comments
  - Skip protected code blocks

**Fixtures:**

- `null_check_samples.cpp` - Sample C++ code with various patterns

**Example Test:**

```python
def test_detects_direct_dereference_without_check():
    """Should detect vehicle-> without preceding null check."""
    code = """
    Vehicle* vehicle = getVehicle();
    vehicle->setSomething(value);  // ERROR: no null check
    """
    issues = analyze_code(code)
    assert len(issues) == 1
    assert "null check" in issues[0].description
```

**Running:**

```bash
pytest tools/tests/test_vehicle_null_check.py -v
pytest tools/tests/test_vehicle_null_check.py::TestNullCheckAnalyzer::test_detects_direct_dereference_without_check
```

## Test Fixtures

Located in `tools/tests/fixtures/`:

| File | Purpose |
|------|---------|
| **test_spec.yaml** | Valid FactGroup YAML specification |
| **test_spec_invalid.yaml** | Invalid YAML for error handling |
| **null_check_samples.cpp** | C++ code samples with null access patterns |

**Creating New Fixtures:**

1. Place file in `fixtures/` directory
2. Reference with `fixtures/filename`
3. Use for samples and test data

## Installation

Install pytest and dependencies:

```bash
# Via install_python.py
python tools/setup/install_python.py test

# Or manually
pip install pytest
```

## Running Tests

### All Tests

```bash
pytest tools/tests/
```

### With Coverage Report

```bash
pytest --cov=tools --cov-report=html tools/tests/
open htmlcov/index.html
```

### Watch Mode (requires pytest-watch)

```bash
pip install pytest-watch
ptw tools/tests/
```

### Specific Tests

```bash
# Single file
pytest tools/tests/test_common.py

# Single class
pytest tools/tests/test_common.py::TestPatterns

# Single method
pytest tools/tests/test_common.py::TestPatterns::test_fact_member_pattern

# Pattern matching
pytest -k "fact_member" tools/tests/
```

### With Markers

```bash
# Run only fast tests (if marked)
pytest -m "not slow" tools/tests/

# Run only integration tests
pytest -m "integration" tools/tests/
```

## Adding New Tests

**1. Create test file:**

```python
# tools/tests/test_mymodule.py
import pytest
from tools.mymodule import MyClass

class TestMyClass:
    """Test MyClass functionality."""

    def test_basic_operation(self):
        """Should perform basic operation."""
        obj = MyClass()
        result = obj.do_something()
        assert result == expected_value

    @pytest.fixture
    def sample_data(self):
        """Provide sample data."""
        return {"key": "value"}

    def test_with_fixture(self, sample_data):
        """Test using fixture."""
        obj = MyClass()
        result = obj.process(sample_data)
        assert result is not None
```

**2. Create fixture if needed:**

```python
# In tools/tests/fixtures/
# mydata.json
{
  "samples": [{"id": 1}, {"id": 2}]
}
```

**3. Use fixture in test:**

```python
import json

@pytest.fixture
def my_data():
    with open("fixtures/mydata.json") as f:
        return json.load(f)

def test_process_data(my_data):
    assert len(my_data["samples"]) == 2
```

## Debugging Failed Tests

### Verbose Output

```bash
pytest -vv tools/tests/test_common.py
```

### Stop on First Failure

```bash
pytest -x tools/tests/
```

### Drop into Debugger on Failure

```bash
pytest --pdb tools/tests/test_mymodule.py
```

### Print Statements

```bash
pytest -s tools/tests/  # Shows print() output
```

### Show Local Variables

```bash
pytest -l tools/tests/  # Shows locals on failure
```

## Continuous Integration

Tests run automatically in CI (GitHub Actions):

- On every pull request
- On commits to main branches
- Generates coverage reports
- Reports test results

See `.github/workflows/` for configuration.

## Best Practices

1. **Use Descriptive Names**
   - `test_detects_xyz_pattern` > `test_1`
   - Class names describe what's being tested

2. **One Assertion Per Test**
   - Makes failures clearer
   - Easier to debug
   - Better test isolation

3. **Use Fixtures for Setup**
   - Reusable test data
   - Automatic cleanup
   - Clear test dependencies

4. **Test Edge Cases**
   - Empty inputs
   - Invalid data
   - Boundary conditions
   - Concurrent access (if applicable)

5. **Keep Tests Fast**
   - Mock external dependencies
   - Use fixtures for expensive setup
   - Mark slow tests with `@pytest.mark.slow`

## Troubleshooting

### ImportError: No module named 'tools'

**Solution**: Run from repository root:
```bash
cd /path/to/qgroundcontrol
pytest tools/tests/
```

### Fixture Not Found

**Solution**: Place fixture in `tools/tests/fixtures/` and reference properly:
```python
import os
fixture_path = os.path.join(os.path.dirname(__file__), "fixtures", "myfile.yaml")
```

### Tests Pass Locally but Fail in CI

**Solution**:
- Check Python version: `python --version`
- Install test dependencies: `pip install -e ./tools[test]`
- Run exact CI commands: See `.github/workflows/`

## Dependencies

| Package | Purpose |
|---------|---------|
| **pytest** ≥7.0 | Test runner |
| **pytest-cov** | Coverage reporting |
| **pyyaml** | YAML parsing (FactGroup specs) |

## See Also

- [Pytest Documentation](https://docs.pytest.org/)
- [QGroundControl Developer Guide](https://dev.qgroundcontrol.com/)
- [Python Testing Best Practices](https://docs.python-guide.org/writing/tests/)
