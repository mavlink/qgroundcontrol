# 构建与测试工作流（含 Windows）

命令以仓库 [justfile](../../../../justfile)、[cmake/presets](../../../../cmake/presets/Windows.json) 和 [test/README.md](../../../../test/README.md) 为准。版本真相在 `.github/build-config.json`（Qt、CMake、GStreamer 最低版本）。

## 环境前提

- C++20 编译器 + CMake ≥ 3.25 + Ninja（Windows 推荐）+ Qt 6（preset 用 `QT_ROOT_DIR` 指向 Qt 安装，如 `C:\Qt\6.11.1\msvc2022_64`，该目录下需有 `lib\cmake\Qt6\qt.toolchain.cmake`）。
- GStreamer（视频功能）；首次配置会通过 CPM 拉取其余依赖。
- `just`（推荐安装）：`python tools/setup/install_python.py dev`（会在 `.venv` 中装好 rust-just）。
- 首次构建必须初始化 submodule（`just configure` 已包含；手动则 `git submodule update --init --recursive`）。
- 查看本机解析到的版本与路径：`just info`；依赖体检：`python tools/check_deps.py`。

## Windows / PowerShell 推荐路径（CMake preset）

preset 生成器为 **Ninja Multi-Config**，产物目录 `build/Windows`（arm64 为 `build/Windows-arm64`）。

```powershell
# 1. 设置 Qt 路径（当前会话生效；建议写入用户环境变量）
$env:QT_ROOT_DIR = "C:\Qt\6.11.1\msvc2022_64"

# 2. 配置（拉 submodule 需先手动执行 git submodule update --init --recursive）
cmake --preset Windows

# 3. 构建（Debug / Release 两个 build preset 都有）
cmake --build --preset Windows-debug      # 或 --preset Windows（Release）

# 4. 测试：Multi-Config 必须用 -C 指定配置
ctest --test-dir build/Windows -C Debug --output-on-failure -L Unit
ctest --test-dir build/Windows -C Debug -R MyTest --output-on-failure   # 单个测试
```

注意：

- PowerShell 中用 `python`（不是 `python3`）；不要用 `&&` 连接命令，用 `;` 或分行执行。
- Multi-Config 生成器下 `ctest` 必须带 `-C Debug|Release`，否则可能找不到测试。
- 不要用 `cmd /c`；脚本执行策略受限，优先 `.cmd`/`.exe`，不要修改执行策略。

## 跨平台 just 工作流（与 CI 一致）

```powershell
# justfile 默认 QT 路径是 gcc_64（Linux 习惯），Windows 先覆盖 QT_DIR
$env:QT_DIR = "C:\Qt\6.11.1\msvc2022_64"

just configure      # = submodules + python tools/configure.py -B build -t Debug --testing --qt-root $QT_DIR
just build          # cmake --build build --config Debug --parallel <CPU 核数>
just release        # 配置并构建 Release
just test           # cd build; ctest --output-on-failure -L "Unit|Integration" -LE "Flaky|Network"
just lint           # pre-commit run --all-files
just format-fix     # 应用 clang-format（QML 用 qmlformat，随 pre-commit）
just analyze        # clang-tidy / clazy / cppcheck / qmllint 等静态分析
just coverage       # 覆盖率报告（产物在 build-coverage/）
just check          # lint + test，声明完成前必过
just clean          # tools/clean.py，支持 --cache / --all / --dry-run
just info           # 打印 Qt/CMake/GStreamer/构建目录等解析值
```

覆盖参数（环境变量）：`BUILD_TYPE=Release`、`JOBS=N`、`LABELS="Unit"`、`EXCLUDE="Network"`，例如：

```powershell
$env:LABELS="Unit"; just test
```

## 测试循环（由窄到宽）

1. 开发中只跑相关测试：`ctest -R <正则>` 或直接调测试可执行体。
2. 相关目录标签全绿后再跑全量 `Unit`。
3. 收尾：`just check`（与 CI 门禁一致：`ctest --output-on-failure -L Unit`）。

### 通过 QGroundControl 可执行体跑（unittest 构建）

```powershell
.\build\Windows\Debug\QGroundControl.exe --list-tests
.\build\Windows\Debug\QGroundControl.exe --list-tests --label=Unit,Utilities
.\build\Windows\Debug\QGroundControl.exe --unittest:MyTest
.\build\Windows\Debug\QGroundControl.exe --unittest:MyTest -- -v2 _specificMethod   # 单方法
.\build\Windows\Debug\QGroundControl.exe --unittest:MyTest --unittest-output:results.xml
.\build\Windows\Debug\QGroundControl.exe --unittest:MyTest --unittest-stress:50
```

### 常用 ctest 过滤

```powershell
ctest --test-dir build/Windows -C Debug -L Unit --output-on-failure
ctest --test-dir build/Windows -C Debug -LE "Flaky|Network"
ctest --test-dir build/Windows -C Debug -R "Camera|Mission"
ctest --test-dir build/Windows -C Debug --rerun-failed
```

### Ninja 便利目标（在构建目录内）

`check`（全部）、`check-unit`、`check-integration`、`check-fast`、`check-ci`（排除 Flaky/Network）、
`check-flaky`（反复跑到失败）、分类目标 `check-vehicle` / `check-missionmanager` / `check-comms` /
`check-mavlink` / `check-utilities`。

### 标签与超时

- 注册：`add_qgc_test(Name LABELS Unit [Vehicle|MissionManager|Comms|MAVLink|...] [Slow] [SERIAL] [RESOURCE_LOCK X] [TIMEOUT n])`。
- 超时时长（非 sanitizer）：`Unit` 60s、`Integration` 120s、`Slow` 180s、其余默认 90s；sanitizer 构建自动放大。
- `Network` 标签 CI 与 `just test` 默认排除；`SERIAL` 自动串行。
- 等待超时一律用 `TestTimeout::shortMs()/mediumMs()/longMs()`（本地 1/5/30s，CI 自动约 2×）。

## Sanitizer / 调试

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON -DQGC_ENABLE_ASAN=ON -DQGC_ENABLE_UBSAN=ON
# TSAN 单独跑：-DQGC_ENABLE_TSAN=ON
```

- 测试详细输出：`$env:QGC_TEST_VERBOSE="1"` 后加 `--unittest:MyTest`。
- 严格日志模式：测试中任何未预期/未抑制的日志都会 `QFAIL`，用 `expectLogMessage`/`ignoreLogMessage` 处理。

## 平台部署相关

- Android：`android/`（Gradle、Manifest），preset 见 `cmake/presets/Android.json`，CI 见 `.github/workflows/android.yml`。
- iOS/macOS/Linux：`deploy/` 与对应 preset、workflow。
- Windows 打包图标等：`deploy/windows/`。
- OEM 定制构建：参考 `custom-example/`（`QGC_CUSTOM_DIR` 机制，见根 `CMakeLists.txt`）。

## 常见问题

- **configure 找不到 Qt**：确认 `QT_ROOT_DIR`（preset）或 `QT_DIR`（just）指向含 `lib/cmake/Qt6` 的 Qt 版本目录。
- **链接错误像缺文件**：新加的 `.cc/.h` 没进模块 `CMakeLists.txt` 的 `target_sources`；新 QML 没进 `QML_FILES`。
- **ctest 报无测试（Windows）**：Multi-Config 漏了 `-C Debug/Release`，或配置时未启用测试（`--testing` / `-DQGC_BUILD_TESTING=ON`）。
- **submodule 缺失导致配置失败**：`git submodule update --init --recursive` 后重新 configure。
- **pre-commit 失败**：先 `just format-fix`，其余按钩子名对照 `.pre-commit-config.yaml` 修复，不要 `--no-verify`。
