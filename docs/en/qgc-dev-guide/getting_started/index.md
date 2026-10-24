# Getting Started with Source and Builds

This topic explains how to get the _QGroundControl_ source code and build it natively, in a container, or in a virtual machine.
It also provides information about optional or OS-specific functionality.

## Daily Builds

If you just want to test (and not debug) a recent build of _QGroundControl_ you can use the [Daily Build](../../qgc-user-guide/releases/daily_builds.md).
Versions are provided for all platforms.

## Source Code

Source code for _QGroundControl_ is kept on [GitHub](https://github.com/mavlink/qgroundcontrol).
It is [dual-licensed under Apache 2.0 and GPLv3](https://github.com/mavlink/qgroundcontrol/blob/master/.github/COPYING.md).

To get the source files, clone the repo (or your fork):

```sh
git clone -j8 https://github.com/mavlink/qgroundcontrol.git
```

## Build QGroundControl

_QGroundControl_ uses [Qt](https://www.qt.io) as its cross-platform support library and is built exclusively with CMake (qmake is no longer supported).
You can build it:

- [Natively](#native-builds) on your own machine.
- In a [container](../getting_started/container.md), without installing any requirements locally (Linux builds and a VS Code Dev Container).
- In a disposable Linux [virtual machine](#virtual-machines) with Vagrant or Multipass.

### Native Builds

_QGroundControl_ builds are supported for Windows, macOS, Linux, Android and iOS.

The required version of Qt is {{ $frontmatter.qt_version }} **(only)**.

::: warning
**Do not use any other version of Qt!**
QGC has been thoroughly tested with the specified version of Qt ({{ $frontmatter.qt_version }}).
Other Qt versions carry a significant risk of injecting bugs that affect stability and safety (even if QGC compiles).
:::

The Qt version, the minimum CMake version, and the pinned GStreamer, Android and Xcode versions are all defined in [`.github/build-config.json`](https://github.com/mavlink/qgroundcontrol/blob/master/.github/build-config.json).
`just info` prints the Qt, CMake and GStreamer versions for your checkout.
For more information see: [Qt 6 supported platform list](https://doc.qt.io/qt-6/supported-platforms.html).

#### Prerequisites

- **Python 3.10+** and **[uv](https://docs.astral.sh/uv/)**: QGC's code generators and tools run in a locked Python environment in `tools/.venv`, which CMake creates with uv the first time you configure.
  On Linux and macOS, `python3 tools/setup/install_uv.py` installs uv; on Windows, follow the [uv installation guide](https://docs.astral.sh/uv/getting-started/installation/).
- **CMake 3.28 or newer** and **Ninja**: the dependency scripts below install them on Linux and macOS, and the `just` recipes run CMake from the locked tool environment.
  If you run `cmake`/`qt-cmake` yourself, make sure the `cmake` on your `PATH` is new enough. Ubuntu 24.04's package is; on older distributions, such as Ubuntu 22.04 or Debian 12, use the CMake from _Developer and Designer Tools_ in the Qt Online Installer or `pip install cmake`.
- **[just](https://just.systems)** (recommended): the command runner used for the everyday build, test and lint commands.
  Install it, together with prek (the pre-commit hook runner) and the other developer tools, into `tools/.venv`:

  ```sh
  python3 tools/setup/install_python.py dev
  . tools/.venv/bin/activate   # Windows PowerShell: tools\.venv\Scripts\Activate.ps1
  ```

  Run `just` with no arguments to list every recipe; the [tools README](https://github.com/mavlink/qgroundcontrol/blob/master/tools/README.md) describes them.

#### Install Qt

You **must install Qt as described below** instead of using pre-built packages from, say, a Linux distribution.

To install Qt with the Qt Online Installer:

1. Download and run the [Qt Online Installer](https://www.qt.io/download-qt-installer-oss).
   - **Ubuntu:** Set the downloaded file to executable using `chmod +x`. It may also be necessary to install _libxcb-cursor0_.
1. On the _Installation Folder_ page select _Custom Installation_.
1. On the _Select Components_ page:
   - If you don't see _Qt {{ $frontmatter.qt_version }}_ listed check the _Archive_ checkbox and click _Filter_.
   - Under Qt -> _Qt {{ $frontmatter.qt_version }}_ select:
     - **Windows**: MSVC 2022 \<_arch_\> - where \<_arch_\> is the architecture of your machine
     - **Mac**: Desktop
     - **Linux**: Desktop gcc 64-bit
     - **Android**: Android (plus the Desktop kit for your host)
   - Select all _Additional Libraries_.
   - Deselect Qt Design Studio.

Alternatively, `tools/setup/install_qt.py` installs exactly the Qt version and modules CI uses (with QGC's own installer on Linux, which needs CMake, and [aqtinstall](https://github.com/miurahr/aqtinstall) elsewhere), for example on Linux:

```sh
python3 tools/setup/install_qt.py install --from-config --host linux --arch linux_gcc_64 --outdir ~/Qt
```

Use `--host mac --arch clang_64` on macOS, or `--host windows --arch win64_msvc2022_64` on Windows.

#### Install Additional Packages

Run the dependency script from the root of the cloned repository (`cd qgroundcontrol`). It is the same script CI uses:

- **Linux** (Debian/Ubuntu, Fedora or Arch) and **macOS:** `python3 tools/setup/install_dependencies` (or `just deps`).
  The platform is detected automatically; override it with `--platform debian|fedora|arch|macos`.
  It installs system packages with apt, dnf or pacman (using `sudo`), or with Homebrew plus the GStreamer framework on macOS.
- **Windows:** `python tools/setup/install_dependencies --platform windows`, from an Administrator prompt.
  By default it installs only GStreamer (x64 only, from the QGC dependency mirror).
  Optional flags: `--nsis` installs NSIS (needed to build the installer), `--msvc` installs the Visual Studio 2022 Build Tools C++ workload, and `--vulkan` installs the Vulkan SDK.

Useful options: `--dry-run` shows what would be installed, `--list` lists the packages by category, and on Linux `--category <name>` installs a single category (`core`, `qt`, `gstreamer`, `sdl`, `audio` or `misc`; on Debian/Ubuntu also `android`, which is `core` plus the JDK for Gradle).

::: tip
Android dependencies are best set up by following the Qt documentation: read [Qt 6 for Android](https://doc.qt.io/qt-6/android.html) carefully, then continue with [Getting Started with Qt 6 for Android](https://doc.qt.io/qt-6/android-getting-started.html).
See [Android and iOS](#android-and-ios) below for the QGC-specific settings.
:::

#### OS-Specific Functionality

Optional features that depend on the operating system and user-installed libraries are described below.
Features can be forcibly enabled or disabled by passing `-D` options to CMake (for example `-DQGC_ENABLE_GST_VIDEOSTREAMING=OFF`).

- **Video Streaming/GStreamer:** Installed by the dependency script on Linux. On Windows, macOS, Android and iOS, CMake also downloads the GStreamer SDK at configure time if it isn't already installed. See [Video Streaming](https://github.com/mavlink/qgroundcontrol/blob/master/src/VideoManager/VideoReceiver/GStreamer/README.md) for details.
  When cross-compiling for Windows ARM64 from an x64 host, disable video streaming with `-DQGC_ENABLE_GST_VIDEOSTREAMING=OFF` (this is what CI does): the ARM64 GStreamer SDK installer cannot run on an x64 host, so the automatic download fails.
- **Windows Installer:** Building the Windows installer requires [NSIS](https://nsis.sourceforge.io/Download), which the dependency script installs when run with `--nsis`.

#### Install Visual Studio Compiler (Windows Only) {#vs}

An MSVC 2022 C++ toolchain is required. Either of the following works:

- The [Visual Studio 2022 Build Tools](https://visualstudio.microsoft.com/downloads/) with the _C++ build tools_ workload (this is what CI uses, and what the dependency script installs when run with `--msvc`), or
- [Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/) with the _Desktop development with C++_ workload, if you also want the IDE.

::: info
Visual Studio is ONLY used to get the compiler. Building _QGroundControl_ is done using [just](#just), [CMake](#cmake) or [Qt Creator](#qt-creator) as outlined below; run command-line builds from an _x64 Native Tools Command Prompt for VS 2022_.
:::

#### Build using just {#just}

From the root of the repository:

```sh
just configure   # Configure a Debug build in build/ (CMake "default" preset)
just build       # Build, using all cores (JOBS=N limits parallel jobs)
just run         # Launch the application you just built
just test        # Run the unit tests
```

- `just release` configures and builds Release instead. `BUILD_TYPE` selects other build types, for example `BUILD_TYPE=RelWithDebInfo just configure build`.
- `tools/configure.py` (which `just configure` runs) uses the `qt-cmake` from the newest Qt it finds under `~/Qt` (or `C:\Qt` on Windows). Set `QT_ROOT_DIR` to a Qt kit directory, such as `~/Qt/{{ $frontmatter.qt_version }}/gcc_64`, to choose a different one.
- `just check` runs the lint checks, the Python tooling tests and the unit tests; see [Code Submission](../contribute/index.md) and [Unit Tests](../contribute/unit_tests.md).

#### Build using CMake on CLI {#cmake}

Example commands to build a default QGC and run it afterwards:

1. Change into the repository folder (see [Source Code](#source-code) above):

   ```sh
   cd qgroundcontrol
   ```

1. Configure:

   ```sh-vue
   ~/Qt/{{ $frontmatter.qt_version }}/gcc_64/bin/qt-cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   ```

   Change the directory for `qt-cmake` to match your install location for Qt and the kit you want to use.

   **Windows**: use the corresponding kit path, for example `C:\Qt\{{ $frontmatter.qt_version }}\msvc2022_64\bin\qt-cmake.bat`, from an _x64 Native Tools Command Prompt for VS 2022_.

   **Mac**: To Sign/Notarize/Staple the QGC app bundle, add `-DQGC_MACOS_SIGN_WITH_IDENTITY=ON` to the configure command line. During the `install` phase the following environment variables will need to be available:

   - `QGC_MACOS_SIGNING_IDENTITY` - Signing identity for your Developer ID certificate which must be in the keychain
   - `QGC_MACOS_NOTARIZATION_USERNAME` - Username for your Apple Developer Account
   - `QGC_MACOS_NOTARIZATION_PASSWORD` - App specific password for Notarization from your Apple Developer Account
   - `QGC_MACOS_NOTARIZATION_TEAM_ID` - Apple Developer Account Team ID

1. Build:

   ```sh
   cmake --build build --config Debug
   ```

   Ninja builds in parallel by default; add `--parallel <N>` to limit the number of jobs.

1. Run the QGroundControl binary that was just built:

   ```sh
   ./build/Debug/QGroundControl
   ```

   On Windows: `build\Debug\QGroundControl.exe`; on macOS: `./build/Debug/QGroundControl.app/Contents/MacOS/QGroundControl`.

##### CMake Presets

[`CMakePresets.json`](https://github.com/mavlink/qgroundcontrol/blob/master/CMakePresets.json) defines the configurations CI builds, so local and CI builds match.
The platform presets (`Linux-debug`, `macOS-debug`, `Windows-debug`, `Linux-sanitizers`, `Linux-coverage`, `iOS-simulator`, and so on) take the Qt kit from `QT_ROOT_DIR` and build in `build/<preset>`:

```sh-vue
export QT_ROOT_DIR=~/Qt/{{ $frontmatter.qt_version }}/gcc_64
cmake --list-presets=all                 # Show every preset
cmake --workflow --preset Linux-debug    # Configure, build and test
```

Cross-compiling presets need more environment variables; see the [CMake development guide](https://github.com/mavlink/qgroundcontrol/blob/master/cmake/README.md#presets) for each preset family's requirements.
Put machine-local presets in `CMakeUserPresets.json`, which Git ignores.

#### Building using Qt Creator {#qt-creator}

1. Launch _Qt Creator_, select Open Project and select the **CMakeLists.txt** file.
1. On the _Configure Project_ page it should default to the version of Qt you just installed using the instruction above. If not select that kit from the list and click _Configure Project_.

   :::tip
   Don't forget to check boxes in case you want to build a Release instead of Debug, or check the other types. To create the installation file go to the _Deploy Settings_ tab, click the _Add Deploy Step_ menu button, select _CMake Install_ and set at least `--config Release` as its argument.
   :::

1. Build using the "hammer" icon. After that, in order to deploy the build, use the "play" icon. Or use the menu Build on top for a detailed alternative.

#### Building using VS Code {#vs-code}

Run `just vscode` to install the workspace settings, build/test tasks and debugger launch configurations from the tracked templates in `.vscode/` (existing files are never overwritten), then install the recommended extensions when VS Code prompts you.
To develop inside a container instead, see [VS Code Dev Container](../getting_started/container.md#vs-code-dev-container).

#### Android and iOS

- **Android:** Install Qt for Android together with the matching desktop Qt (the _host_ Qt), plus the Android SDK, NDK and JDK versions listed under `android` in `build-config.json`.
  On Debian/Ubuntu, `python3 tools/setup/install_dependencies --category android` installs the build tools and the matching OpenJDK.
  The `Android` and `Android-debug` presets read `QT_TARGET_ROOT_DIR` (Qt for Android), `QT_HOST_PATH` (desktop Qt), `ANDROID_NDK` and `ANDROID_MIN_SDK` from the environment.
  The build produces an APK; `cmake --build <build-dir> --target aab` also builds an Android App Bundle.
- **iOS** (macOS host with Xcode): Install Qt for iOS together with the matching macOS Qt.
  The `iOS`, `iOS-debug` and `iOS-simulator` presets read `QT_ROOT_DIR` (Qt for iOS) and `QT_HOST_PATH` (macOS Qt); `iOS-simulator` builds a Debug app for the x86_64 iOS Simulator.

## Building QGC Installation Files

You can additionally create installation files for _QGroundControl_ from a Release build:

- **Linux AppImage and macOS DMG:** run the install step:

  ```sh
  cmake --install build --config Release
  ```

  Use the same build directory you passed to `-B` when configuring, and a configuration (`Release`/`Debug`) that you actually built.

- **Windows installer:** build the `qgc-package` target, which creates the NSIS installer (requires NSIS, installed by the dependency script when run with `--nsis`):

  ```sh
  cmake --build build --config Release --target qgc-package
  ```

- **Linux `.deb` and `.rpm` packages:** with `QT_ROOT_DIR` set, run the `Linux-deb` or `Linux-rpm` workflow preset, for example `cmake --workflow --preset Linux-deb`.

## Build Caching

QGC uses two build caches, both enabled automatically at configure time and stored in the source
tree so they survive build directory deletion:

- **ccache** caches compiler output (`.cache/ccache/`). Used if `ccache` is installed and on your `PATH`
  (the Linux and macOS dependency scripts install it); `sccache` is used instead if only it is found,
  with precompiled headers turned off because sccache can reuse a stale GCC precompiled header.
- **moccache** caches Qt moc output (`.cache/moccache/`). QGC has a large number of moc-processed
  headers, so on clean builds, branch switches, and rebuilds after deleting the build directory a
  warm moccache skips the entire moc phase. Controlled by the `QGC_USE_MOCCACHE` CMake option
  (ON by default).

Neither cache requires any setup. To bypass moccache for a single build set `MOCCACHE_DISABLE=1`,
or configure with `-DQGC_USE_MOCCACHE=OFF` to turn it off entirely.

### moccache Details

moccache (the `tools/moccache/` package) is a content-addressed cache wired in automatically as the
`AUTOMOC_EXECUTABLE` via a launcher script generated at configure time. Cached moc output
is keyed on moc version + arguments + input content + transitive include contents + the input's
path relative to the output directory (moc embeds that relative path as an `#include` in its
output, so build trees laid out at a different depth intentionally don't share entries). Entries
are otherwise shared across build trees: build-dir paths are rewritten to a token, like ccache's
`base_dir`. The launcher runs `tools/moccache_launcher.py`, which imports the `moccache` package so
Python reuses its cached bytecode instead of recompiling it on every moc run.

Cache misses fall through to the real moc and never fail the build; corrupt or stale entries are
caught on every use, when each recorded include is re-validated by content hash unless its mtime,
ctime and size still match the values recorded with that hash.

Environment variables (set by the launcher, overridable at build time):

| Variable            | Default                  | Purpose                                                 |
| ------------------- | ------------------------ | ------------------------------------------------------- |
| `MOCCACHE_DIR`      | `<repo>/.cache/moccache` | Cache location (persisted by CI, like ccache)           |
| `MOCCACHE_MAX_SIZE` | `1G`                     | LRU auto-trim threshold (trims after misses)            |
| `MOCCACHE_DISABLE`  | unset                    | Pass through to real moc (no caching)                   |
| `MOCCACHE_STATS`    | unset                    | Log hits/misses; print this build's stats after linking |

```sh
MOCCACHE_STATS=1 just build                             # This build's hit/miss stats print after linking
MOCCACHE_BASEDIR=$PWD/build python3 ./tools/moccache_launcher.py --show-stats --build-dir build --verbose  # Per-miss reasons for the last build
python3 ./tools/moccache_launcher.py --zero-stats       # Reset the stats log
python3 ./tools/moccache_launcher.py --trim --max-size 1G  # Explicitly trim the cache (LRU)
MOCCACHE_DISABLE=1 just build                           # Bypass the cache for one build
```

Each miss is logged with a reason, and the report explains each kind inline, so you can tell
expected misses from cache problems: `miss-first-seen`, `miss-header-changed`,
`miss-include-changed` and `miss-predefs-changed` are expected; `miss-cache-evicted` means the
cache is too small (raise `MOCCACHE_MAX_SIZE`); `miss-args-changed` and
`miss-output-depth-differs` mean build trees differ in a way that defeats sharing and are worth
investigating (`--verbose` shows the first differing argument per miss). See the docstring in
`tools/moccache/__init__.py` for the full list. `--markdown` prints the same report as tables, with
the cache size; CI build jobs add it to the job summary.

## Virtual Machines

Both VM options provision an Ubuntu guest, install the dependencies and Qt with the repository's own scripts, and build a Release AppImage.

### Vagrant

[Vagrant](https://www.vagrantup.com/) can be used to build and run _QGroundControl_ within a Linux virtual machine (the build can also be run on the host machine if it is compatible).

1. [Download](https://www.vagrantup.com/downloads.html) and [Install](https://www.vagrantup.com/docs/getting-started/) Vagrant and a provider (VirtualBox, VMware or libvirt).
   With libvirt the guest boots via UEFI: install the `ovmf` package (Debian/Ubuntu), or point `QGC_VM_UEFI_LOADER` at your distribution's combined OVMF firmware image.
1. From the [`deploy/vagrant`](https://github.com/mavlink/qgroundcontrol/tree/master/deploy/vagrant) directory of the repository run `vagrant up`.
   This provisions an Ubuntu desktop guest (with Qt Creator) that shares the repository as `/vagrant`, builds QGC, and copies the build to `shadow_build/` in your checkout.
1. To use the graphical environment run `vagrant reload`.

To reproduce the CI Vagrant job on an Ubuntu host, run `deploy/vagrant/run_vagrant.py`. Its `install` step sets up Vagrant, libvirt/KVM, UEFI firmware and the vagrant-libvirt plugin; the default `build` step runs a headless VM, copies the AppImage to `OUTPUT_DIR` (default: the current directory) and destroys the VM:

```sh
python3 deploy/vagrant/run_vagrant.py install
OUTPUT_DIR=artifacts python3 deploy/vagrant/run_vagrant.py
```

`QGC_VM_CPUS`, `QGC_VM_MEM_MB` and `QGC_VM_UP_TIMEOUT` (seconds, default 150 minutes) adjust the VM size and the `vagrant up` time limit.
The script refuses to run while another Vagrant machine (such as the desktop VM above) exists in `deploy/vagrant`; remove it first with `vagrant destroy`.

### Multipass

[Multipass](https://canonical.com/multipass) builds in a throwaway Ubuntu VM and copies the AppImage to `OUTPUT_DIR` (default: the current directory):

```sh
QGC_SOURCE_DIR=$PWD OUTPUT_DIR=artifacts python3 deploy/multipass/run_multipass.py
```

Without `QGC_SOURCE_DIR` the VM clones upstream `master` instead of building your checkout.
`MP_CPUS` (default 4), `MP_MEM` (8G), `MP_DISK` (25G) and `MP_IMAGE` size and select the VM, and `MP_TIMEOUT` (default 900 seconds) bounds `multipass launch`.
The instance (`MP_NAME`, default `qgc`) must not already exist, and is deleted when the build finishes.
