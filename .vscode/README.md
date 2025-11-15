# VSCode Configuration for QGroundControl

Pre-configured VSCode workspace settings, tasks, and debugging configurations.

## Quick Setup

### 1. Install Recommended Extensions
VSCode will automatically prompt to install recommended extensions from `extensions.json`.

**Manual install**:
```bash
# Open Command Palette (Ctrl+Shift+P)
# Run: Extensions: Show Recommended Extensions
```

**Essential extensions:**
- **ms-vscode.cpptools** OR **llvm-vs-code-extensions.vscode-clangd** - C++ support
- **ms-vscode.cmake-tools** - CMake integration
- **editorconfig.editorconfig** - EditorConfig support

### 2. Copy Template Files
```bash
cd .vscode
cp settings.json.template settings.json
cp tasks.json.template tasks.json
cp launch.json.template launch.json
cp c_cpp_properties.json.template c_cpp_properties.json
```

### 3. Adjust Paths
Edit the copied files and update Qt paths for your system:

**Linux:**
```json
"${env:HOME}/Qt/6.10.0/gcc_64"
```

**macOS (Intel):**
```json
"${env:HOME}/Qt/6.10.0/clang_64"
```

**macOS (Apple Silicon):**
```json
"${env:HOME}/Qt/6.10.0/macos"
```

## Configuration Files

### `settings.json` - Workspace Settings
**Features:**
- C++ and CMake configuration
- Qt/QML paths
- Auto-formatting on save
- Search exclusions
- Git settings

**Choose C++ Language Server:**

**Option A: clangd (Recommended)**
```json
{
  "clangd.path": "/usr/bin/clangd",
  "C_Cpp.intelliSenseEngine": "disabled"
}
```

**Option B: Microsoft C/C++**
```json
{
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
  "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json"
}
```

### `tasks.json` - Build Tasks
**Available tasks:**
- **Configure (Debug)** - Setup Debug build
- **Configure (Release)** - Setup Release build
- **Build (Debug)** - Build Debug (default: Ctrl+Shift+B)
- **Build (Release)** - Build Release
- **Clean Build** - Remove build directory
- **Rebuild (Debug)** - Clean + Build
- **Run Unit Tests** - Execute all tests
- **Check Code Quality** - Run linters
- **Fix Code Quality** - Auto-fix issues
- **Run Pre-commit** - Run pre-commit hooks
- **Build Documentation** - Build VitePress docs
- **Serve Documentation** - Dev server for docs

**Run a task:**
- Menu: Terminal → Run Task
- Shortcut: Ctrl+Shift+B (runs default build task)

### `launch.json` - Debug Configurations
**Available configurations:**
- **Debug QGroundControl** - Standard debugging
- **Debug QGroundControl (No Build)** - Skip build step
- **Run Unit Tests (Debug)** - Debug all tests
- **Run Specific Test (Debug)** - Debug single test
- **Debug with Valgrind** - Memory leak detection
- **Attach to Process** - Attach debugger to running instance

**Start debugging:**
- Menu: Run → Start Debugging
- Shortcut: F5

### `c_cpp_properties.json` - IntelliSense Config
**Only needed if using Microsoft C/C++ extension** (not clangd).

Configures include paths and compiler settings for IntelliSense.

### `extensions.json` - Recommended Extensions
VSCode automatically prompts to install these extensions.

**Categories:**
- C/C++ Development (cpptools or clangd, cmake-tools)
- Qt/QML Support
- Code Quality (editorconfig, spell checker)
- Git (gitlens, git-graph)
- Documentation (markdown)
- Utilities (todo-tree, better-comments)

## Workflow Examples

### Standard Development
1. **Open project**: `code /path/to/qgroundcontrol`
2. **Configure**: Ctrl+Shift+P → "CMake: Configure"
3. **Build**: Ctrl+Shift+B
4. **Debug**: F5

### Build & Test
1. **Run task**: Ctrl+Shift+P → "Tasks: Run Task" → "Run Unit Tests"
2. Or debug tests: F5 → Select "Run Unit Tests (Debug)"

### Code Quality Check
1. **Before commit**: Ctrl+Shift+P → "Tasks: Run Task" → "Check Code Quality"
2. **Auto-fix**: Run task → "Fix Code Quality"
3. **Pre-commit**: Run task → "Run Pre-commit"

### Debug Specific Test
1. **F5** → Select "Run Specific Test (Debug)"
2. **Enter test name** (e.g., "FactSystemTest")
3. **Set breakpoints** in test code
4. **Debug**

## Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Build | Ctrl+Shift+B |
| Debug/Run | F5 |
| Stop Debugging | Shift+F5 |
| Restart Debugging | Ctrl+Shift+F5 |
| Step Over | F10 |
| Step Into | F11 |
| Step Out | Shift+F11 |
| Toggle Breakpoint | F9 |
| Command Palette | Ctrl+Shift+P |
| Quick Open File | Ctrl+P |
| Find in Files | Ctrl+Shift+F |
| Terminal | Ctrl+` |

## CMake Tools Integration

### Configure
```
Ctrl+Shift+P → CMake: Configure
```

### Select Kit
```
Ctrl+Shift+P → CMake: Select a Kit
Choose: Qt 6.10.0 (gcc_64 or macos)
```

### Build
```
Ctrl+Shift+P → CMake: Build
Or: Ctrl+Shift+B (default task)
```

### Change Build Type
```
Ctrl+Shift+P → CMake: Select Variant
Choose: Debug, Release, RelWithDebInfo, MinSizeRel
```

## Debugging Tips

### Qt Creator Pretty Printers
For better Qt type visualization in gdb:
```bash
# Install Qt Creator helpers
sudo apt install qtcreator  # Linux
# Or use tools/qt6.natvis for Visual Studio
```

### QML Debugging
Enable QML debugging in build:
```bash
qt-cmake -B build -DQGC_DEBUG_QML=ON
```

### Enable Verbose Output
Launch QGC with environment variables:
```json
{
  "environment": [
    {"name": "QSG_INFO", "value": "1"},
    {"name": "QT_LOGGING_RULES", "value": "*.debug=true"}
  ]
}
```

## Troubleshooting

### IntelliSense not working
**Solution 1** (clangd):
```bash
# Generate compile_commands.json
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

**Solution 2** (Microsoft C/C++):
```
Ctrl+Shift+P → C/C++: Edit Configurations (JSON)
Verify includePath and compilerPath
```

### CMake configure fails
```
Ctrl+Shift+P → CMake: Delete Cache and Reconfigure
```

### Qt not found
Edit `settings.json` and update `cmake.cmakePath`:
```json
{
  "cmake.cmakePath": "/absolute/path/to/qt-cmake"
}
```

### Debug symbols not loading
Build with Debug symbols:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

### Extensions not auto-installing
```
Ctrl+Shift+P → Extensions: Show Recommended Extensions
Install manually
```

## Platform-Specific Notes

### Linux
- Use `gcc_64` Qt kit
- clangd path: `/usr/bin/clangd`
- gdb path: `/usr/bin/gdb`

### macOS (Intel)
- Use `clang_64` Qt kit
- clangd path: `/usr/bin/clangd`
- lldb path: `/usr/bin/lldb`

### macOS (Apple Silicon)
- Use `macos` Qt kit
- Everything else same as Intel

### Windows
See Windows-specific VSCode setup in Developer Guide.

## See Also

- [QUICKSTART.md](../QUICKSTART.md) - Quick build guide
- [DEV_CONFIG.md](../.github/DEV_CONFIG.md) - Configuration reference
- [tools/README.md](../tools/README.md) - Build scripts
- [VSCode C++ Docs](https://code.visualstudio.com/docs/languages/cpp)
- [CMake Tools Docs](https://github.com/microsoft/vscode-cmake-tools/tree/main/docs)
