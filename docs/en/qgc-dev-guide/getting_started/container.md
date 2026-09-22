# Building using Containers

The community created a docker image that makes it much easier to build a Linux-based QGC application.
This can give you a massive boost in productivity and help with testing.

## About the Container

The Container is located in the `./deploy/docker` directory.
It's based on __Ubuntu 24.04__. However, always check the first lines of the Dockerfile in the aforementioned directory to have zero surprises.

It checks and pre-installs all the dependencies at build time, including Qt, thanks to the use of scripts from the directory `./tools/setup`.
The main advantage of using the container is the usage of the `CMake` build system and its many improvements over `qmake`.

## Building the Container

### Script

To build the container using the script, run this command in the qgc root directory

```sh
python3 deploy/docker/run_docker.py build ubuntu
```

### Manual

if you want to Build using the container manually, then you first have to build the image.
You can accomplish this using docker, running the following script from the root of the QGC source code directory.

```sh
docker build --target linux --file ./deploy/docker/Dockerfile -t qgc-ubuntu-docker .
```

::: info
The `-t` flag is essential.
Keep in mind this is tagging the image for later reference since you can have multiple builds of the same container
:::

The native Linux target selects the matching Qt kit on amd64 and ARM64 hosts,
including Apple Silicon Docker Desktop. The separate `linux-cross` target remains
an amd64-to-ARM64 cross-compilation environment.

## Building QGC using the Container

To use the container to build QGC, you first need to define a directory to save the artifacts.
We recommend you create a `build` directory on the source tree and then run the docker image using the tag provided above as follows, from the root directory:

```sh
mkdir build
docker run --rm -v ${PWD}:/project/source -v ${PWD}/build:/project/build qgc-ubuntu-docker
```

::: info
For up to date docker commands and options, reference the run scripts in [`deploy/docker`](https://github.com/mavlink/qgroundcontrol/tree/master/deploy/docker).

:::

::: info
If using the script to build the Linux image on a Windows host, you would need to reference the PWD differently.
On Windows the docker command is:

```sh
docker run --rm -v %cd%:/project/source -v %cd%/build:/project/build qgc-ubuntu-docker
```

:::

Depending on your system resources, or the resources assigned to your Docker Daemon, the build step can take some time.

## Development Container

The existing VS Code development container is named __qgc-dev__. It includes Clang,
clang-tidy, clang-scan-deps, clangd, and Clazy built against the same LLVM.
`.github/build-config.json` supplies the LLVM major version, checksum-verified Clazy
revision, and Qt version used by CI. The image build uses `deploy/docker/install_analysis.py`;
ccache uses the pinned, signature-verified release from `.github/scripts/ccache_helper.py`.
The locked Python `dev` profile includes `just`, build, lint and test tools.
Git and the GitHub CLI are prebuilt too, without separate devcontainer feature installs.
Linux GStreamer libraries come from the existing system dependency installer,
satisfying the configured minimum. The amd64 image also includes Android tooling;
Apple SDKs are not included.
Existing application builder tags and Docker Hub flows are unchanged.

`ghcr.io/mavlink/qgc-dev:latest` is one multi-platform OCI index for native
`linux/amd64` and `linux/arm64`. Docker selects the correct architecture automatically.
The `qgc-dev.yml` workflow updates `latest` only for meaningful image-input changes
on `master`. A published stable QGC release gets its exact existing tag, for example
`ghcr.io/mavlink/qgc-dev:v5.0.0`, built from that released commit, without changing
`latest`. Retries preserve an existing matching stable tag and reject a changed source.
The workflow summary records `ghcr.io/mavlink/qgc-dev@sha256:<digest>` for pinning.

Relevant PRs and manual __Run workflow__ dispatches build both architectures without
publishing. Dispatch does not require changed files. Drafts and prereleases do not
publish stable images. Release automation explicitly calls the same publisher because
`GITHUB_TOKEN`-created release events do not start another workflow.
Older releases lacking this definition cannot be backfilled using current master.
No application or analyzer workflow adopts this image in this change.

Qt, Python, and analysis executables are on `PATH` for non-login shells and non-root users.
Image checks verify native executable architectures, configured tool versions, Clazy's
LLVM linkage, Python imports, and real Qt compilation plus clang-tidy/Clazy execution
as a non-root user. The prebuilt environment is used by `just` without runtime
SDK provisioning or Python synchronization.

### Android development

The __linux/amd64__ variant of this same image includes the configured Java JDK,
Android command-line tools, platform tools, SDK platform, build-tools, official NDK,
and Qt Android kits for __arm64-v8a__, __armeabi-v7a__, and __x86_64__ (emulator
target). Versions and Qt modules come from `.github/build-config.json`, using the
same provisioning helper as the existing Android application builder.
SDK licenses are accepted during image creation.

The official NDK's Linux host tools are x86-64 only. Native __linux/arm64__
qgc-dev provides Linux desktop development and analysis, __not Android builds__.
On Apple Silicon select `--platform linux/amd64`; Docker Desktop supplies emulation
(Rosetta when enabled). This still uses `ghcr.io/mavlink/qgc-dev`, not another image.
The image does not include an emulator, system images, Android Studio, or the x86
Qt target. Running an emulator requires a separate appropriately accelerated host.

Java, `sdkmanager`, `adb`, `aapt2`, `zipalign`, and `apksigner` work in non-login
shells as the non-root image user. `JAVA_HOME`, `ANDROID_SDK_ROOT`,
`ANDROID_NDK_ROOT`/`ANDROID_NDK`, and `ANDROID_BUILD_TOOLS_DIR` point at preinstalled
tools. Qt target kits are under `/opt/qt-android/<ABI>`. Desktop `QT_ROOT_DIR`,
`qt-cmake`, `just configure`, and `just release` remain native Linux defaults.
Use `qgc-android <ABI> <command>` to supply the Android preset's target Qt root,
host Qt path, and configured SDK settings for that command only:

```sh
docker run --rm -it --platform linux/amd64 \
  -v "$PWD:/workspaces/qgroundcontrol" ghcr.io/mavlink/qgc-dev:latest \
  bash -c 'qgc-android arm64-v8a cmake --preset Android -B build/android-arm64 &&
           qgc-android arm64-v8a cmake --build build/android-arm64 --parallel 4'
```

Initialize submodules and writable checkout ownership as for the desktop example.
Use separate build directories for each ABI. Android SDK/NDK/Qt installation is
not repeated at runtime. A full QGC build can still download project dependencies,
Android GStreamer/OpenSSL, and Gradle/Maven packages; those caches and release-signing
credentials are not bundled.

The existing shared image smoke test compiles and links a small Qt Android library
for all three installed ABIs and checks their ELF architectures. It also exercises
resource compilation, APK alignment, signing with a disposable test key, and signature
verification offline as non-root. This is a bounded toolchain test, not a full QGC
APK build or emulator boot test. Native ARM64 checks the explicit Android limitation
instead, without installing unusable cross-compiler binaries.

### Local image builds

To build and load the host platform using the same Bake target as CI, run from the
repository root (use `linux/amd64` on an x86-64 Docker host):

```sh
docker buildx bake -f deploy/docker/docker-bake.hcl qgc-dev \
  --set qgc-dev.platform=linux/arm64 --load
docker run --rm -it -v "$PWD:/workspaces/qgroundcontrol" qgc-dev:local
```

To exercise x86-64 locally on Apple Silicon, select `linux/amd64` in Bake and add
`--platform linux/amd64` to `docker run`. Docker Desktop provides x86-64 emulation,
using Rosetta when enabled.

For a full native Linux QGC build, initialize the checkout's submodules, make the
checkout and build directory writable by the container user, and run the existing recipes:

```sh
git submodule update --init --recursive
mkdir -p build
docker run --rm -it -v "$PWD:/workspaces/qgroundcontrol" qgc-dev:local \
  bash -c 'git config --global --add safe.directory /workspaces/qgroundcontrol && just release'
```

On Linux, map ownership to the development user's UID/GID (1000 by default), or
set `USER_UID`/`USER_GID` when building the image. `just` uses detected CPU parallelism;
set `JOBS` to limit it. The result is `build/Release/QGroundControl`, a Linux artifact
even when Docker runs on macOS or Windows. The image workflow runs the same
container build and bounded smoke checks for both architectures.

After configuring a compilation database and generating headers and autogen targets
as described in the [tools guide](https://github.com/mavlink/qgroundcontrol/blob/master/tools/README.md#centralized-configuration),
run `python tools/analyze.py --tool clang-tidy --build-dir build` or
`python tools/analyze.py --tool clazy --build-dir build`. ccache accelerates compilation,
not the analyzers' AST scans.

## Troubleshooting

### Windows: 'bash\r': No such file or directory

This error indicates that a Linux script is being run with Windows line endings, which might occur if `git` was configured to use Windows line endings:

```sh
 > [4/7] RUN /tmp/qt/install-qt-linux.sh:
#9 0.445 /usr/bin/env: 'bash\r': No such file or directory
```

One fix is to force Linux line endings using the command:

```sh
git config --global core.autocrlf false
```

Then update/recreate your local repository. For, example, use:

```sh
git rm --cached -r .
git reset --hard
```
