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
   git commit -m "feat: brief description"
   ```

6. **Push to your fork**

   ```bash
   git push origin feature/my-new-feature
   ```

7. **Create a Pull Request** from your fork to `mavlink/qgroundcontrol:master`

---

## Coding Standards

Follow [CODING_STYLE.md](../CODING_STYLE.md) for naming, formatting, C++20 features, QML style, and logging conventions. Run `clang-format` and `pre-commit run` before committing.

### Architecture Patterns

QGroundControl has several core architecture patterns you must follow. See [CODING_STYLE.md](../CODING_STYLE.md) for full details with code examples:

- **Fact System**: ALL vehicle parameters use Facts — never create custom parameter storage
- **Multi-Vehicle**: ALWAYS null-check `activeVehicle()` before use
- **Firmware Plugin**: Use `vehicle->firmwarePlugin()` for firmware-specific behavior
- **QML Integration**: Use `QML_ELEMENT`/`QML_SINGLETON`/`QML_UNCREATABLE` macros, `Q_PROPERTY` for bindings

---

## Testing Requirements

See [test/TESTING.md](../test/TESTING.md) for the complete testing guide, including base classes, CTest labels, `MultiSignalSpy`, and coverage.

**Key points:**

- Add unit tests for new functionality in `test/` mirroring `src/` structure
- Use the `UnitTest` base class (or `VehicleTest`, `MissionTest`, etc.)
- Run `ctest --output-on-failure -L Unit` before submitting
- Test on multiple platforms and both PX4/ArduPilot when applicable

### Pre-commit Checks

Run before committing:

```bash
make lint                    # or: just lint
pre-commit run --all-files   # full check
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

All contributions must be compatible with QGroundControl's **dual-license system** (Apache 2.0 AND GPL v3). Your code must be original or from a compatible license (BSD, MIT, Apache 2.0).

See [COPYING.md](COPYING.md) for full license details, compatible licenses, and contact information.

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
