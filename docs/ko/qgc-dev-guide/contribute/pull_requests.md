# Pull Requests

All pull requests go through the QGC CI build system (GitHub Actions), which:

- Builds QGC for each supported platform (Linux, Windows, macOS, Android and iOS), and in [containers](../getting_started/container.md).
  Builds will fail if there are compiler warnings in QGC code.
- Runs the [unit tests](../contribute/unit_tests.md) on Linux (with AddressSanitizer/UndefinedBehaviorSanitizer and with coverage instrumentation) and on a macOS Debug build.
- Runs the pre-commit checks (`just lint`) on code changes, and the documentation lint (`just lint-docs`), link check and site build on documentation changes.

Signing secrets are not available to pull request builds: Android builds are signed with a throwaway debug keystore and macOS builds are ad-hoc signed.

Follow [Conventional Commits](https://www.conventionalcommits.org/) for commit messages, and use the repository's pull request template to describe the change and how you tested it.

## Automatic Release Note Generation

Release notes are generated from the following GitHub labels, which should be set on Pull Requests as appropriate:

- "RN: MAJOR FEATURE"
- "RN: MINOR FEATURE"
- "RN: IMPROVEMENT"
- "RN: REFACTORING"
- "RN: BUGFIX"
- "RN: NEW BOARD SUPPORT"

Variants of these labels ending in "- CUSTOM BUILD" indicate that the change is associated with the custom build architecture.
