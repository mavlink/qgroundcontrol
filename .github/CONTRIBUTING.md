# Contributing to QGroundControl

Thank you for considering contributing to QGroundControl! This guide will help you get started with contributing code, reporting issues, and improving documentation.

## Table of Contents

1. [Getting Started](#getting-started)
2. [How to Contribute](#how-to-contribute)
3. [Coding Standards](#coding-standards)
4. [Testing Requirements](#testing-requirements)
5. [Pull Request Process](#pull-request-process)
6. [License Requirements](#license-requirements)

---

## Getting Started

### Prerequisites

Before you begin, please:

1. Read the [Developer Guide](https://dev.qgroundcontrol.com/en/)
2. Review the [Build Instructions](https://dev.qgroundcontrol.com/en/getting_started/)
3. Familiarize yourself with the [Architecture Patterns](#architecture-patterns) in this guide

### Development Environment

- **Language**: C++20 with Qt 6.10+ framework
- **Build System**: CMake 3.25+
- **Platforms**: Windows, macOS, Linux, Android, iOS
- **IDE**: Qt Creator (recommended), VS Code, or your preferred IDE

#### Quick Setup with direnv (optional)

If you use [direnv](https://direnv.net/), the repository includes a `.envrc` that automatically:
- Activates the Python virtual environment
- Adds Qt to your PATH (if installed)
- Exports build configuration variables
- Enables ccache if available

```bash
direnv allow  # Run once after cloning
```

---

## How to Contribute

### Reporting Issues

Before creating a new issue:

1. **Search existing issues** to avoid duplicates
2. **Provide complete information**:
   - QGroundControl version
   - Operating system and version
   - Detailed steps to reproduce
   - Log files (from `~/.local/share/QGroundControl/`)
   - Screenshots or videos if applicable

**Create an issue**: <https://github.com/mavlink/qgroundcontrol/issues>

**For security vulnerabilities**: See our [Security Policy](SECURITY.md) for responsible disclosure procedures.

### Suggesting Enhancements

Feature requests are welcome! Please:

1. Check if the feature already exists or has been requested
2. Explain the use case and benefits
3. Consider implementation complexity
4. Be prepared to contribute code if possible

### Contributing Translations

QGroundControl uses [Crowdin](https://crowdin.com/project/qgroundcontrol) for community translations. See [tools/translations/README.md](../tools/translations/README.md) for details on how translations are managed.

### Contributing Code

1. **Fork the repository**

   ```bash
   git clone https://github.com/YOUR-USERNAME/qgroundcontrol.git
   cd qgroundcontrol
   ```

2. **Create a feature branch**

   ```bash
   git checkout -b feature/my-new-feature
   ```

3. **Make your changes** following our [coding standards](#coding-standards)

4. **Test your changes thoroughly**
   - Run unit tests: `./qgroundcontrol --unittest`
   - Test on all relevant platforms when possible
   - Test with both PX4 and ArduPilot if applicable

5. **Commit your changes**

   ```bash
   git add .
   git commit -m "Add feature: brief description"
   ```

6. **Push to your fork**

   ```bash
   git push origin feature/my-new-feature
   ```

7. **Create a Pull Request** from your fork to `mavlink/qgroundcontrol:master`

---

## Coding Standards

For the complete coding style guide with examples, see [CODING_STYLE.md](../CODING_STYLE.md).

### C++ Guidelines

- **Standard**: C++20
- **Framework**: Qt 6 guidelines
- **Naming Conventions**:
  - Classes: `PascalCase`
  - Methods/functions: `camelCase`
  - Private members: `_leadingUnderscore`
  - Constants: `ALL_CAPS` or `kPascalCase`

- **Always use braces** for if/else/for/while statements

  ```cpp
  // Good
  if (condition) {
      doSomething();
  }

  // Bad
  if (condition) doSomething();
  ```

- **Defensive coding**:
  - Always null-check pointers before use
  - Validate all inputs
  - Use Q_ASSERT for debug-build development checks only (compiled out in release builds)
  - Always use defensive error handling in production code paths (never rely on Q_ASSERT)
  - Handle errors gracefully in production code

- **Code formatting**:
  - Run `clang-format` before committing
  - Follow `.clang-format`, `.clang-tidy`, `.editorconfig` in repo root
  - See `CodingStyle.h`, `CodingStyle.cc`, `CodingStyle.qml` for examples
  - 4 spaces for indentation (no tabs)

### QML Guidelines

- Follow Qt QML coding conventions
- Use type annotations
- Prefer declarative over imperative code
- See `src/QmlControls/QGCButton.qml` for examples

### Logging

Use Qt logging categories:

```cpp
Q_DECLARE_LOGGING_CATEGORY(MyComponentLog)
QGC_LOGGING_CATEGORY(MyComponentLog, "qgc.component.name")

qCDebug(MyComponentLog) << "Debug message:" << value;
qCWarning(MyComponentLog) << "Warning message";
qCCritical(MyComponentLog) << "Critical error";
```

### Architecture Patterns

#### Fact System (Most Important!)

The Fact System handles ALL vehicle parameters. Never create custom parameter storage.

```cpp
// Access parameters (always null-check!)
Fact* param = vehicle->parameterManager()->getParameter(-1, "PARAM_NAME");
if (param && param->validate(newValue, false).isEmpty()) {
    param->setCookedValue(newValue);  // Use cookedValue for UI (with units)
    // param->rawValue() for MAVLink/storage
}
```

**Key classes:**

- `Fact` - Single parameter with validation, units, metadata
- `FactGroup` - Hierarchical container (handles MAVLink via `handleMessage()`)
- `FactMetaData` - JSON-based metadata (min/max, enums, descriptions)

**Rules:**

- Wait for `parametersReady` signal before accessing
- Use `cookedValue` (display) vs `rawValue` (storage)
- Metadata in `*.FactMetaData.json` files

#### Multi-Vehicle Support

Always null-check the active vehicle:

```cpp
Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
if (!vehicle) return;

// Other managers
SettingsManager::instance()->appSettings()->...
LinkManager::instance()->...
```

#### Firmware Plugin System

Use FirmwarePlugin for firmware-specific behavior:

```cpp
// FirmwarePlugin - Firmware behavior (flight modes, capabilities)
vehicle->firmwarePlugin()->flightModes();
vehicle->firmwarePlugin()->isCapable(capability);

// AutoPilotPlugin - Vehicle setup UI
// VehicleComponent - Individual setup items (Radio, Sensors, Safety)
```

#### QML/C++ Integration

```cpp
Q_OBJECT
QML_ELEMENT           // Creatable in QML
QML_SINGLETON         // Singleton
QML_UNCREATABLE("")   // C++-only

Q_PROPERTY(Type name READ getter WRITE setter NOTIFY signal)
Q_INVOKABLE void method();
Q_ENUM(EnumType)
```

---

## Testing Requirements

### Unit Tests

- Add unit tests for new functionality
- Place tests in `test/` directory mirroring `src/` structure
- Use Qt Test framework with `UnitTest` base class
- Run tests before submitting:

  ```bash
  ./qgroundcontrol --unittest
  ```

### Manual Testing

Test your changes on:

- Multiple platforms (Windows, macOS, Linux if possible)
- Both PX4 and ArduPilot firmware (if applicable)
- Different vehicle types (multirotor, fixed-wing, VTOL, rover)

### Pre-commit Checks

Run before committing:

```bash
# Using Makefile or justfile (recommended)
make lint        # or: just lint

# Format code
clang-format -i path/to/changed/files.cc

# Run pre-commit hooks (optional)
pre-commit run --all-files
```

See [tools/README.md](../tools/README.md) for all available development commands.

---

## Pull Request Process

### Before Submitting

1. **Rebase on latest master**

   ```bash
   git fetch upstream
   git rebase upstream/master
   ```

2. **Ensure all tests pass**
3. **Update documentation** if needed
4. **Write a clear PR description**:
   - What problem does it solve?
   - How was it tested?
   - Breaking changes (if any)
   - Screenshots for UI changes

### PR Requirements

- ✅ All CI checks must pass
- ✅ Code follows style guidelines
- ✅ Tests added for new features
- ✅ No unrelated changes
- ✅ Commit messages are clear and descriptive

### Review Process

- Maintainers will review your PR
- Address feedback in new commits (don't force-push during review)
- Once approved, a maintainer will merge your PR

### After Merging

- Delete your feature branch
- Your contribution will appear in the next release
- Thank you for contributing! 🎉

---

## License Requirements

### Dual-License Requirement

**Important**: All contributions to QGroundControl must be compatible with our **dual-license system** (Apache 2.0 AND GPL v3).

### What This Means

- **Your code must be original** or from a compatible license
- **Compatible licenses**: BSD 2-clause, BSD 3-clause, MIT, Apache 2.0
- **Incompatible licenses**: GPL-only, proprietary, copyleft-only licenses

By contributing, you agree that:

1. Your contributions are your original work or properly licensed
2. You grant QGroundControl rights under **both** Apache 2.0 and GPL v3 licenses
3. You have the right to submit the contribution

### License Background

QGroundControl uses a dual-license system:

#### Apache License 2.0

- Permissive license
- Allows use in proprietary applications
- Allows distribution via app stores
- **Requires commercial Qt license**

Full text: [LICENSE-APACHE](../LICENSE-APACHE)

#### GNU General Public License v3 (GPL v3)

- Copyleft license
- Ensures software remains open source
- **Can use open-source Qt**
- Users can use later GPL versions (v3 is minimum for contributions)

Full text: [LICENSE-GPL](../LICENSE-GPL)

### Questions About Licensing

If you have questions about licensing, please contact:

- Lorenz Meier: <lm@qgroundcontrol.org>

For more details, see [COPYING.md](COPYING.md).

---

## Additional Resources

- **User Manual**: <https://docs.qgroundcontrol.com/en/>
- **Developer Guide**: <https://dev.qgroundcontrol.com/en/>
- **Support Guide**: For help and community resources, see [SUPPORT.md](SUPPORT.md)
- **Discussion Forum**: <https://discuss.px4.io/c/qgroundcontrol>
- **Discord**: <https://discord.gg/dronecode>

---

## Code of Conduct

QGroundControl is part of the Dronecode Foundation. Please follow our [Code of Conduct](CODE_OF_CONDUCT.md).

---

Thank you for contributing to QGroundControl! Your efforts help make drone control accessible to everyone.
