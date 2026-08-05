---
qt_version: 6.11.1
---

# 从源码开始并构建

本主题说明如何获取 _QGroundControl_ 源代码并在本机或在 _Vagrant_ 环境中构建它。 本主题还提供其他可选功能信息及特定于操作系统的功能信息。
本主题还提供其他可选功能信息及特定于操作系统的功能信息。

## 每日构建

如果您只是想测试 (而不是调试) 最近生成的 _QGroundControl_ ，那么请使用[Daily build](../../qgc-user-guide/releases/daily_builds.md)。 官方提供了适用于所有平台的版本。
官方提供了适用于所有平台的版本。

## 源代码

Source code for _QGroundControl_ is kept on [GitHub](https://github.com/mavlink/qgroundcontrol).
它采用 [Apache 2.0 和 GPLv3 双重授权](https://github.com/mavlink/qgroundcontrol/blob/master/.github/COPYING.md)。

To get the source files, clone the repo (or your fork):

```sh
git clone -j8 https://github.com/mavlink/qgroundcontrol.git
```

## 构建QGroundControl开发环境

### 使用容器

我们支持使用存储库源代码树上的容器进行 Linux 构建，这可以帮助您开发和部署 QGC 应用程序，而无需在本地环境中安装任何要求。

[容器指南](../getting_started/container.md)

### 原生构建

_QGroundControl_ 支持macos、linux、windows 和 Android 平台的构建。 理论上可以为iOS创建一个 QGC 版本，但不再支持作为标准构建。
_QGroundControl_ 使用 [Qt](http://www.qt.io)作为其跨平台支持库。

所需的 Qt 版本为 {{ $frontmatter.qt_version }} **(必须无误)**。

::: warning
**请勿使用任何其他版本的 Qt！**
QGC 已通过指定 Qt 版本（{{ $frontmatter.qt_version }}）的全面测试。
其它的 Qt 版本很可能会注入影响稳定和安全的 bug (即使QGC 编译通过)。
:::

更多信息请看: [Qt 6 支持平台列表](https://doc.qt.io/qt-6/supported-platforms.html)。

#### 安装Qt

您**必须像下面描述的那样安装Qt** ，而不是使用预构建的软件包，例如Linux发行版。

如何安装Qt：

1. 下载并运行[Qt Online Installer](https://www.qt.io/download-qt-installer-oss)
   - **Ubuntu:**
     - 使用以下命令将下载的文件设置为可执行文件：`chmod + x`
     - It may also be necessary to install _libxcb-cursor0_.

2. 在 _Installation 文件夹页面选择"自定义安装"

3. 在 _选择组件_ 页面：

- I you don't see _Qt {{ $frontmatter.qt_version }}_ listed check the _Archive_ checkbox and click _Filter_.
- Under Qt -> _Qt {{ $frontmatter.qt_version }}_ select:
  - **Windows**: MSVC 2022 \<_arch_\> - where \<_arch_\> is the architecture of your machine
  - **Mac**: Desktop
  - **Linux**: Desktop gcc 64-bit
  - **Android**: Android
- Select all _Additional Libraries_
- Deselect QT Design Studio

1. 安装附加软件包(特殊平台)

   Run these from the root of the cloned repository (`cd qgroundcontrol`):

   - **Ubuntu:** `python3 tools/setup/install_dependencies --platform debian`

   - **Fedora:** `sudo dnf install speech-dispatcher SDL2-devel SDL2 systemd-devel patchelf`

   - **Arch Linux:** `pacman -Sy speech-dispatcher patchelf`

   - **Mac:** `python3 tools/setup/install_dependencies --platform macos`

   - **Windows:** `python tools/setup/install_dependencies --platform windows`

     This is the same script used by CI. By default it installs only GStreamer (x64 only, from the QGC dependency mirror). Optional flags: `--nsis` installs NSIS (needed to build the installer), `--msvc` installs the Visual Studio 2022 Build Tools C++ workload, and `--vulkan` installs the Vulkan SDK.

   - **Android:** Installing dependencies for Android is quite involved. You are better off using Qt documentation for Android setup instructions. Read [Qt 6 for Android](https://doc.qt.io/qt-6/android.html) carefully. Continue with [Getting Started with Qt 6 for Android](https://doc.qt.io/qt-6/android-getting-started.html).

2. Install OS-Specific Functionalities

   ::: info
   QGC is built exclusively with CMake; qmake builds are no longer supported.
   Optional features that are dependent on the operating system and user-installed libraries are linked/described below.
   These features can be forcibly enabled/disabled by passing additional `-D` options to CMake (e.g. `-DQGC_ENABLE_GST_VIDEOSTREAMING=OFF`).
   :::

   - **Video Streaming/GStreamer:** - installed by the dependency script above, or downloaded automatically at CMake configure time on Windows/macOS if not already present. See [Video Streaming](https://github.com/mavlink/qgroundcontrol/blob/master/src/VideoManager/VideoReceiver/GStreamer/README.md) for details. Note: when cross-compiling for Windows ARM64 from an x64 host, disable video streaming with `-DQGC_ENABLE_GST_VIDEOSTREAMING=OFF` (this is what CI does) — the ARM64 GStreamer SDK installer cannot run on an x64 host, so the automatic download will fail.
   - **Windows Installer:** - building the Windows installer requires [NSIS](https://nsis.sourceforge.io/Download), which the dependency script above installs when run with `--nsis`.

#### Install Visual Studio Compiler (Windows Only) {#vs}

An MSVC 2022 C++ toolchain is required. Either of the following works:

- The [Visual Studio 2022 Build Tools](https://visualstudio.microsoft.com/downloads/) with the _C++ build tools_ workload (this is what CI uses, and what the dependency script above installs when run with `--msvc`), or
- [Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/) with the _Desktop development with C++_ workload, if you also want the IDE.

  ::: info
  Visual Studio is ONLY used to get the compiler. Building _QGroundControl_ is done using [Qt Creator](#qt-creator) or [cmake](#cmake) directly as outlined below.
  :::

#### 使用 Qt Creator {#qt-creator} 进行构建

1. 启动 _Qt Creator_，选择 Open Project 并选择 **CMakeLists.txt** 文件。

2. 在 _Configure Project_ 页面上，它应该默认您刚刚使用上面的说明安装的 Qt 版本。 如果不从列表中选择该套件，然后点击 _Configure Project_。

   :::tip
   Don't forget to check boxes in case you want to build a Release instead of Debug, or check the other types. To create the installation file go to the "Deploy Settings" Tab, click in the menu button "Add Deploy Step", select "CMake Install" and as argument you must set at least `--config Release`.
   :::

3. Build using the "hammer" icon. After that, in order to deploy the build, use the "play" icon. Or use the menu Build on top for a detailed alternative.

#### 在CLI（命令行界面）使用 CMake {#cmake} 进行构建

构建默认的 QGC 示例命令并在此后运行它：

1. Make sure you cloned the repository before, see chapter _Source Code_ above and switch into the repository folder:

   ```sh
   cd qgroundcontrol
   ```

2. 配置：

   ```sh
   ~/Qt/{{ $frontmatter.qt_version }}/gcc_64/bin/qt-cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   ```

   Change the directory for `qt-cmake` to match your install location for Qt and the kit you want to use.

   **Windows**: use the corresponding kit path, e.g. `C:\Qt\{{ $frontmatter.qt_version }}\msvc2022_64\bin\qt-cmake.bat`, from a _x64 Native Tools Command Prompt for VS 2022_.

   **Mac**: To Sign/Notarize/Staple the QGC app bundle, add `-DQGC_MACOS_SIGN_WITH_IDENTITY=ON` to the configure command line. During the `install` phase the following environment variables will need to be available:

   - `QGC_MACOS_SIGNING_IDENTITY` - Signing identity for your Developer ID certificate which must be in the keychain
   - `QGC_MACOS_NOTARIZATION_USERNAME` - Username for your Apple Developer Account
   - `QGC_MACOS_NOTARIZATION_PASSWORD` - App specific password for Notarization from your Apple Developer Account
   - `QGC_MACOS_NOTARIZATION_TEAM_ID` - Apple Developer Account Team ID

3. 构建

   ```sh
   cmake --build build --config Debug
   ```

4. Run the QGroundcontrol binary that was just built: `./staging/QGroundControl`

   ```sh
   ./build/Debug/QGroundControl
   ```

   On Windows: `build\Debug\QGroundControl.exe`

### Vagrant

[Vagrant](https://www.vagrantup.com/) 可以在 Linux 虚拟机内构建和运行 _QGroundControl_ (如果兼容，也可以在主机机上运行)。

1. [下载](https://www.vagrantup.com/downloads.html) 并 [安装](https://www.vagrantup.com/docs/getting-started/) Vagrant
2. 在 _QGroundControl_ 仓库的根目录运行 `vagrant up`
3. 若要使用图形环境，请运行 `vagrant reload`

### 所有支持的操作系统的额外构建备注

- **Parallel builds:** You can use the `-j#` option with `cmake --build` to control the number of parallel build jobs.
- **如果你在运行 _QGroundControll_**&#x65F6;遇到此错误: `/usr/lib/x86_64-linux-gnu/libstdc++.so.6: version 'GLIBCXX_3.4.20' not found.`，你需要更新到最新的 _gcc_ ，或者通过使用 `sudo apt-get install libstdc++6` 安装最新的 _libstdc++.6_ 。
- **单元测试:** 若要运行 [单元测试](../contribute/unit_tests.md)，使用 `QGC_UNITEST_BUILD` 定义在 `debug` 模式下构建，然后复制 `deposition / qgroundcontrol-start。 运行测试前，将 `deploy/qgroundcontrol-start.sh\` 脚本复制到debug目录中。

### Build Caching

QGC uses two build caches, both enabled automatically at configure time and stored in the source
tree so they survive build directory deletion:

- **ccache** caches compiler output (`.ccache/`). Used if `ccache` is installed and on your `PATH`.
- **moccache** caches Qt moc output (`.cache/moccache/`). QGC has a large number of moc-processed
  headers, so on clean builds, branch switches, and rebuilds after deleting the build directory a
  warm moccache skips the entire moc phase. Controlled by the `QGC_USE_MOCCACHE` CMake option
  (ON by default; not supported on Windows).

Neither cache requires any setup. To bypass moccache for a single build set `MOCCACHE_DISABLE=1`,
or configure with `-DQGC_USE_MOCCACHE=OFF` to turn it off entirely.

#### moccache Details

moccache (`tools/moccache.py`) is a content-addressed cache wired in automatically as the
`CMAKE_AUTOMOC_EXECUTABLE` via a launcher script generated at configure time. Cached moc output
is keyed on moc version + arguments + input content + transitive include contents + the input's
path relative to the output directory (moc embeds that relative path as an `#include` in its
output, so build trees laid out at a different depth intentionally don't share entries). Entries
are otherwise shared across build trees: build-dir paths are rewritten to a token, like ccache's
`base_dir`.

Cache misses fall through to the real moc and never fail the build; corrupt or stale entries are
re-validated by content hash on every use.

Environment variables (set by the launcher, overridable at build time):

| Variable            | Default                  | Purpose                                                          |
| ------------------- | ------------------------ | ---------------------------------------------------------------- |
| `MOCCACHE_DIR`      | `<repo>/.cache/moccache` | Cache location (persisted by CI, like ccache) |
| `MOCCACHE_MAX_SIZE` | `256M`                   | LRU auto-trim threshold (trims after misses)  |
| `MOCCACHE_DISABLE`  | unset                    | Pass through to real moc (no caching)         |
| `MOCCACHE_STATS`    | unset                    | Append hit/miss lines to `$MOCCACHE_DIR/stats.log`               |

```sh
MOCCACHE_STATS=1 just build                             # Log hits/misses during a build
python3 ./tools/moccache.py --trim --max-size 256M      # Explicitly trim the cache (LRU)
MOCCACHE_DISABLE=1 just build                           # Bypass the cache for one build
```

## 构建 QGC 安装文件

作为正常构建过程的一部分，您还可以为 _QGroundControl_ 创建安装文件。

```sh
cmake --install build --config Release
```

Use the same build directory you passed to `-B` when configuring, and a configuration (`Release`/`Debug`) that you actually built.

On Windows this creates the NSIS installer (requires NSIS, installed by the dependency script when run with `--nsis`).
