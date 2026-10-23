# Contributing to QGroundControl

Thank you for considering contributing to QGroundControl! This guide will help you get started with
contributing code, reporting issues, and improving documentation.

> **AI coding agents** (Claude Code, Codex, etc.): see [AGENTS.md](../AGENTS.md) for the canonical
> agent-facing guide (golden rules, `just` build/test commands, definition of done, commit conventions).
> This document remains the human-facing source of truth for the
> [Architecture Patterns](#architecture-patterns) that AGENTS.md links back to.

## Table of Contents

1. [Getting Started](#getting-started)
2. [How to Contribute](#how-to-contribute)
3. [Coding Standards](#coding-standards)
4. [Testing Requirements](#testing-requirements)
5. [Pull Request Process](#pull-request-process)
6. [License Requirements](#license-requirements)
7. [Additional Resources](#additional-resources)

---

## Getting Started

### Prerequisites

Before you begin, please:

1. Read the [Developer Guide](https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/)
2. Review the [Build Instructions](https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/getting_started/)
3. Familiarize yourself with the [Architecture Patterns](#architecture-patterns) in this guide

### Development Environment

- **Language**: C++20 with the Qt version defined in [`build-config.json`](build-config.json)
- **Build System**: CMake (minimum version defined in [`build-config.json`](build-config.json)); see
  the [CMake development guide](../cmake/README.md) for modules, presets, and focused tests
- **Platforms**: Windows, macOS, Linux, Android, iOS
- **IDE**: Qt Creator (recommended), VS Code, or your preferred IDE
- **Build/test/lint commands**: see [tools/README.md](../tools/README.md) for the `just configure` /
  `build` / `test` / `lint` / `check` workflow

---

## How to Contribute

### Reporting Issues

Before creating a new issue:

1. **Search existing issues** to avoid duplicates
2. **Provide complete information**:
   - QGroundControl version
   - Operating system and version
   - Detailed steps to reproduce
   - Log files (the `Logs` folder in QGroundControl's save path; on desktop the default is
     `~/Documents/QGroundControl/Logs`)
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

QGroundControl uses [Crowdin](https://crowdin.com/project/qgroundcontrol) for community translations. See
[tools/translations/README.md](../tools/translations/README.md) for details on how translations are managed.

### Contributing Code

1. **Fork the repository**

   ```bash
   git clone https://github.com/YOUR-USERNAME/qgroundcontrol.git
   cd qgroundcontrol
   git remote add upstream https://github.com/mavlink/qgroundcontrol.git
   git config blame.ignoreRevsFile .git-blame-ignore-revs  # skip formatting-only commits in blame
   ```

2. **Create a feature branch**

   ```bash
   git checkout -b feature/my-new-feature
   ```

3. **Make your changes** following our [coding standards](#coding-standards)

4. **Test your changes thoroughly**
   - Run `just test` (Unit and Integration labels, as in CI), or
     `ctest --test-dir build --output-on-failure -L Unit` for unit tests only
   - Test on all relevant platforms when possible
   - Test with both PX4 and ArduPilot if applicable

5. **Commit your changes** using [Conventional Commits](../AGENTS.md#commit--review-conventions);
   `prek install` (from the `precommit` tools group) adds hooks that run the lint checks and check the
   message on each commit

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

Follow [CODING_STYLE.md](../CODING_STYLE.md) for naming, formatting, C++20 features, QML style, and logging
conventions. Run `just lint` (`prek run --all-files`) before committing; see
[tools/README.md](../tools/README.md) for all development commands.

### Architecture Patterns

QGroundControl has several core architecture patterns you must follow. See [CODING_STYLE.md](../CODING_STYLE.md)
for full details with code examples:

- **Fact System**: ALL vehicle parameters use Facts — never create custom parameter storage
- **Multi-Vehicle**: ALWAYS null-check `activeVehicle()` / `Vehicle*` before use
- **Firmware Plugin**: Use `vehicle->firmwarePlugin()` for firmware-specific behavior
- **QML Integration**: Use `QML_ELEMENT`/`QML_SINGLETON`/`QML_UNCREATABLE` macros, `Q_PROPERTY` for bindings

---

## Testing Requirements

See [test/README.md](../test/README.md) for the complete testing guide, including base classes, CTest labels,
`MultiSignalSpy`, and coverage.

**Key points:**

- Add unit tests for new functionality in `test/` mirroring `src/` structure
- Use the `UnitTest` base class (or `VehicleTest`, `MissionTest`, etc.)
- Run `just test` before submitting
- Test on multiple platforms and both PX4/ArduPilot when applicable

---

## Pull Request Process

### Before Submitting

1. **Rebase on latest master**

   ```bash
   git fetch upstream
   git rebase upstream/master
   ```

2. **Ensure all checks pass**: `just check` runs lint, the Python tooling tests, and the application tests
3. **Update documentation** if needed
4. **Write a clear PR description**:
   - What problem does it solve?
   - How was it tested?
   - Breaking changes (if any)
   - Screenshots for UI changes

### PR Requirements

- All CI checks must pass
- Code follows style guidelines
- Tests added for new features
- No unrelated changes
- Commit messages are clear and descriptive (Conventional Commits, see
  [AGENTS.md](../AGENTS.md#commit--review-conventions))

### Review Process

- Maintainers will review your PR
- Address feedback; fold fixups into the commits they amend rather than adding a trail of review-fix
  commits (see [AGENTS.md](../AGENTS.md#commit--review-conventions))
- Once approved, a maintainer will merge your PR

### After Merging

- Delete your feature branch
- Your contribution will appear in the next release
- Thank you for contributing!

---

## License Requirements

All contributions must be compatible with QGroundControl's **dual-license system** (Apache 2.0 AND GPL v3).
Your code must be original or from a compatible license (BSD, MIT, Apache 2.0).

See [COPYING.md](COPYING.md) for full license details, compatible licenses, and contact information.

---

## Additional Resources

- **User Manual**: <https://docs.qgroundcontrol.com/master/en/qgc-user-guide/>
- **Developer Guide**: <https://docs.qgroundcontrol.com/master/en/qgc-dev-guide/>
- **Support Guide**: For help and community resources, see [SUPPORT.md](SUPPORT.md)
- **Discussion Forum**: <https://discuss.px4.io/c/qgroundcontrol>
- **Discord**: <https://discord.gg/dronecode>
- **Code of Conduct**: QGroundControl is part of the Dronecode Foundation — see our
  [Code of Conduct](CODE_OF_CONDUCT.md)

---

Thank you for contributing to QGroundControl! Your efforts help make drone control accessible to everyone.
