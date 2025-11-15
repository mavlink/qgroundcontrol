# Development Configuration Files

Quick reference for QGroundControl development tools and configuration files.

## Code Formatting

### `.clang-format` - C++ Code Formatting
Defines C++20 code style (Allman braces, 120 char lines, 4-space indent).

```bash
# Format a file
clang-format -i src/Vehicle/Vehicle.cc

# Check formatting (used in CI)
clang-format --dry-run --Werror src/**/*.cc
```

### `.cmake-format` - CMake Formatting
Formats CMakeLists.txt and .cmake files.

```bash
# Install
pip install cmake-format

# Format
cmake-format -i CMakeLists.txt

# Check (used in pre-commit)
cmake-format --check CMakeLists.txt
```

## Static Analysis

### `.clang-tidy` - C++ Static Analysis
Comprehensive C++20 linting with modernization and performance checks.

```bash
# Run on a file
clang-tidy src/Vehicle/Vehicle.cc

# Run with fixes
clang-tidy --fix src/Vehicle/Vehicle.cc
```

### `.clangd` - Language Server
Configuration for C++ IDE features (code completion, navigation).

**Auto-configured** - Your IDE/editor will use this automatically.

## Editor Configuration

### `.editorconfig` - Cross-Editor Settings
Universal formatting rules for all file types (recognized by most editors).

**Auto-configured** - Supported by VSCode, CLion, Qt Creator, Vim, Emacs, etc.

### `.qmlls.ini` - QML Language Server
Auto-generated QML tooling configuration.

**Automatic**: CMake generates this file with correct paths when you configure the project (`QT_QML_GENERATE_QMLLS_INI=ON` by default).

**Note**: `.qmlls.ini` is gitignored (auto-generated, user-specific).

## Git Hooks

### `.pre-commit-config.yaml` - Pre-commit Hooks
Automated checks before each commit.

```bash
# Install pre-commit framework
pip install pre-commit

# Install hooks (one-time)
pre-commit install

# Run manually on all files
pre-commit run --all-files

# Skip hooks temporarily (not recommended)
git commit --no-verify
```

**Hooks include:**
- YAML/JSON/XML validation
- Trailing whitespace removal
- Line ending normalization (LF)
- Large file detection (>1MB)
- CMake formatting and linting
- Markdown linting

## CI Configuration

### `.github/workflows/` - GitHub Actions
See [workflows/README.md](workflows/README.md) for complete CI/CD documentation.

### `codecov.yml` - Code Coverage
Code coverage configuration (not currently enabled). See [DEVELOPMENT_INFRASTRUCTURE.md](DEVELOPMENT_INFRASTRUCTURE.md#code-coverage) for details.

## Tool Installation

### Ubuntu/Debian
```bash
# Core tools
sudo apt install clang-format clang-tidy clangd cmake-format

# Pre-commit
pip install pre-commit cmake-format

# Optional: QML tools (from Qt installation)
```

### macOS
```bash
# Via Homebrew
brew install clang-format llvm cmake-format

# Pre-commit
pip3 install pre-commit cmake-format
```

### Windows
```bash
# Via LLVM installer + pip
pip install pre-commit cmake-format
```

## IDE Integration

### VSCode
Install extensions:
- **C/C++** (ms-vscode.cpptools) or **clangd** (llvm-vs-code-extensions.vscode-clangd)
- **CMake Tools** (ms-vscode.cmake-tools)
- **EditorConfig** (editorconfig.editorconfig)

### Qt Creator
- **Built-in**: Clang-Format, Clangd, EditorConfig support
- **Settings**: Enable "Format on Save" in Options → C++

### CLion
- **Built-in**: All tools supported natively
- **Settings**: Enable ClangFormat in Settings → Editor → Code Style

## Best Practices

1. **Format before commit** - Run `pre-commit run --all-files` or let hooks auto-format
2. **Fix static analysis warnings** - Address clang-tidy warnings before PR
3. **Match existing style** - When in doubt, match surrounding code
4. **Use EditorConfig** - Ensure your editor respects .editorconfig
5. **Keep configs updated** - Tools evolve; update config files as needed

## Troubleshooting

**Q: Pre-commit hooks slow?**
A: Run specific hooks: `pre-commit run <hook-id>`

**Q: Clang-format breaking code?**
A: Use `// clang-format off` ... `// clang-format on` for special cases

**Q: False positives in clang-tidy?**
A: Add `// NOLINT(check-name)` or disable check in `.clang-tidy`

**Q: IDE not respecting .editorconfig?**
A: Install EditorConfig plugin for your editor

---

**See also:**
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
- [copilot-instructions.md](copilot-instructions.md) - Coding patterns
- [workflows/README.md](workflows/README.md) - CI/CD documentation
