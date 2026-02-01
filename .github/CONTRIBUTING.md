# Contributing to QGroundControl

Thank you for considering contributing to QGroundControl! This guide covers the contribution process. For coding conventions and technical details, see [CODING_STYLE.md](../CODING_STYLE.md).

## Table of Contents

1. [Getting Started](#getting-started)
2. [How to Contribute](#how-to-contribute)
3. [Testing Requirements](#testing-requirements)
4. [Pull Request Process](#pull-request-process)
5. [License Requirements](#license-requirements)

---

## Getting Started

### Prerequisites

Before you begin:

1. Read the [Developer Guide](https://dev.qgroundcontrol.com/en/)
2. Review the [Build Instructions](https://dev.qgroundcontrol.com/en/getting_started/)
3. Familiarize yourself with [CODING_STYLE.md](../CODING_STYLE.md)

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

QGroundControl uses [Crowdin](https://crowdin.com/project/qgroundcontrol) for community translations. See [tools/translations/README.md](../tools/translations/README.md) for details.

### Contributing Code

1. **Fork the repository** and clone your fork
2. **Create a feature branch** from master
3. **Make your changes** following [CODING_STYLE.md](../CODING_STYLE.md)
4. **Test thoroughly** (see [Testing Requirements](#testing-requirements))
5. **Commit with clear messages** (imperative mood: "Add feature", not "Added feature")
6. **Push to your fork** and create a Pull Request

---

## Testing Requirements

### Unit Tests

- Add unit tests for new functionality
- Place tests in `test/` directory mirroring `src/` structure
- Use Qt Test framework with `UnitTest` base class
- Run tests: `./qgroundcontrol --unittest`

### Manual Testing

Test your changes on:

- Multiple platforms (Windows, macOS, Linux if possible)
- Both PX4 and ArduPilot firmware (if applicable)
- Different vehicle types (multirotor, fixed-wing, VTOL, rover)

### Pre-commit Checks

Run before committing:

- `make lint` or `just lint` - Run linting
- `pre-commit run --all-files` - Run all pre-commit hooks

See [tools/README.md](../tools/README.md) for all available development commands.

---

## Pull Request Process

### Before Submitting

1. **Rebase on latest master**
2. **Ensure all tests pass**
3. **Update documentation** if needed
4. **Write a clear PR description**:
   - What problem does it solve?
   - How was it tested?
   - Breaking changes (if any)
   - Screenshots for UI changes

### PR Requirements

- ✅ All CI checks must pass
- ✅ Code follows [CODING_STYLE.md](../CODING_STYLE.md)
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

**Important**: All contributions must be compatible with our **dual-license system** (Apache 2.0 AND GPL v3).

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

| License | Type | Qt Requirement | Use Case |
|---------|------|----------------|----------|
| [Apache 2.0](../LICENSE-APACHE) | Permissive | Commercial Qt license | Proprietary apps, app stores |
| [GPL v3](../LICENSE-GPL) | Copyleft | Open-source Qt | Open source distribution |

### Questions About Licensing

Contact Lorenz Meier: <lm@qgroundcontrol.org>

For more details, see [COPYING.md](COPYING.md).

---

## Additional Resources

- **User Manual**: <https://docs.qgroundcontrol.com/en/>
- **Developer Guide**: <https://dev.qgroundcontrol.com/en/>
- **Support Guide**: [SUPPORT.md](SUPPORT.md)
- **Discussion Forum**: <https://discuss.px4.io/c/qgroundcontrol>
- **Discord**: <https://discord.gg/dronecode>

---

## Code of Conduct

QGroundControl is part of the Dronecode Foundation. Please follow our [Code of Conduct](CODE_OF_CONDUCT.md).

---

Thank you for contributing to QGroundControl!
