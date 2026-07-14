# Contributing to QGroundControl

Thank you for considering contributing to QGroundControl! This guide will help you get started with
contributing code, reporting issues, and improving documentation.

> **AI coding agents** (Claude Code, Codex, etc.): see [AGENTS.md](../AGENTS.md) for the canonical
> agent-facing entry point and definition of done. This document remains the human-facing
> contribution workflow; topic-specific rules live in the guides linked below.

## Table of Contents

1. [Getting Started](#getting-started)
2. [How to Contribute](#how-to-contribute)
3. [Coding Standards](#coding-standards)
4. [Commit Messages](#commit-messages)
5. [Testing Requirements](#testing-requirements)
6. [Pull Request Process](#pull-request-process)
7. [License Requirements](#license-requirements)
8. [Additional Resources](#additional-resources)

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

QGroundControl uses [Crowdin](https://crowdin.com/project/qgroundcontrol) for community translations. See
[tools/translations/README.md](../tools/translations/README.md) for details on how translations are managed.

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
   - Run `just test` (or `ctest --output-on-failure -L Unit` for the unit-test label only)
   - Test on all relevant platforms when possible
   - Test with both PX4 and ArduPilot if applicable

5. **Commit your changes** using [Conventional Commits](#commit-messages)

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
conventions. Run `just lint` (or `pre-commit run --all-files`) before committing.

### Architecture Patterns

The canonical architecture rules and examples are in
[CODING_STYLE.md](../CODING_STYLE.md#common-pitfalls). Its
[Qt/QML integration section](../CODING_STYLE.md#qt6--qml-integration) covers type registration,
properties, signals, and QML structure.

#### Architecture Entry Points

Start with these interfaces when changing vehicle parameters, multi-vehicle behavior, or
firmware-specific integration:

1. `src/FactSystem/Fact.h` — parameter system foundation
2. `src/Vehicle/Vehicle.h` — core vehicle model
3. `src/FirmwarePlugin/FirmwarePlugin.h` — firmware abstraction

### Repository Layout

The primary application modules are under `src/`:

```text
src/
├── Vehicle/          # Vehicle state/comms
├── Comms/            # Link layer (serial, UDP, TCP, Bluetooth)
├── FactSystem/       # Parameter management
├── FirmwarePlugin/   # PX4/ArduPilot abstraction
├── AutoPilotPlugins/ # Vehicle setup UI
├── MissionManager/   # Mission planning
├── MAVLink/          # Protocol handling
├── VideoManager/     # Video pipeline (GStreamer)
├── FlyView/          # In-flight UI
├── PlanView/         # Mission planning UI
├── QmlControls/      # Reusable QML components
└── Settings/         # Persistent settings
```

---

## Commit Messages

Use [Conventional Commits](https://www.conventionalcommits.org/) because the type drives release
automation through `.releaserc.json` and semantic-release.

- Release-triggering types: `feat`, `fix`, `perf`, `revert`
- Non-release types: `docs`, `style`, `chore`, `refactor`, `test`, `build`, `ci`
- Example: `fix(Vehicle): guard null activeVehicle in telemetry handler`

---

## Testing Requirements

See [test/README.md](../test/README.md) for the complete testing guide, including base classes, CTest labels,
`MultiSignalSpy`, and coverage.

Run the checks appropriate to the change. The canonical commands and lint gate are documented in
[tools/README.md](../tools/README.md#quality); test selection and labels are documented in
[test/README.md](../test/README.md#running-tests).

---

## Pull Request Process

### Before Submitting

1. **Rebase on latest master**

   ```bash
   git fetch upstream
   git rebase upstream/master
   ```

2. **Ensure all tests pass** (`just check`)
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
- Commit messages follow [Conventional Commits](#commit-messages)

### Review Process

- Maintainers will review your PR
- Address feedback in new commits (don't force-push during review)
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

- **User Manual**: <https://docs.qgroundcontrol.com/en/>
- **Developer Guide**: <https://dev.qgroundcontrol.com/en/>
- **Support Guide**: For help and community resources, see [SUPPORT.md](SUPPORT.md)
- **Discussion Forum**: <https://discuss.px4.io/c/qgroundcontrol>
- **Discord**: <https://discord.gg/dronecode>
- **Code of Conduct**: QGroundControl is part of the Dronecode Foundation — see our
  [Code of Conduct](CODE_OF_CONDUCT.md)

---

Thank you for contributing to QGroundControl! Your efforts help make drone control accessible to everyone.
