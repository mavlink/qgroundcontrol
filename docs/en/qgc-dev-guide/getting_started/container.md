# Building using Containers

QGroundControl provides Docker images that make it much easier to build a Linux-based QGC application, without installing Qt or any other requirements on your machine.
The same images are used by CI, so a container build is also a quick way to reproduce a CI failure.

## About the Container

The container files are located in the [`deploy/docker`](https://github.com/mavlink/qgroundcontrol/tree/master/deploy/docker) directory:

- `Dockerfile`: a single multi-stage file with a target for each builder image: `linux` (native x86_64 or arm64 AppImage), `linux-cross` (aarch64 cross-build), `android` (APK) and `devcontainer` (see [VS Code Dev Container](#vs-code-dev-container)).
- `variants.json`: the build variants, which select a target, base image and build arguments.
  The default `ubuntu` variant is based on **Ubuntu 24.04**; there are also `ubuntu-2204`, `ubuntu-2604`, `debian`, `fedora`, `arch`, `aarch64` and `android` variants.
- `run_docker.py`: builds a variant's image and runs a QGC build in it.

Each image installs the dependencies and Qt at image build time with the scripts in `tools/setup`, using the versions from `.github/build-config.json`.
Base images are pulled from the [Amazon ECR Public mirror](https://gallery.ecr.aws/docker) of the Docker official images, so builds aren't subject to Docker Hub's anonymous pull limit.

## Building QGC using the Script

To build the image and then build QGC in it, run this command from the QGC root directory:

```sh
uv run --frozen --project tools python deploy/docker/run_docker.py build ubuntu
```

(`just docker` runs the same command.)
[uv](https://docs.astral.sh/uv/getting-started/installation/) supplies the Python 3.12 that the script runs on; a `python3` that is 3.12 or newer can also run `deploy/docker/run_docker.py` directly.
Replace `ubuntu` with any variant from `variants.json`, and optionally append a build type (`Release` by default, or `Debug`, `RelWithDebInfo`, `MinSizeRel`).
The build output, including the AppImage, is written to `build/` (`build-aarch64/` for the `aarch64` variant).

To rebuild with an image you already built, use `run`:

```sh
uv run --frozen --project tools python deploy/docker/run_docker.py run qgc-ubuntu-docker Release
```

The script runs the container as your user, with no extra privileges, and mounts the source and build directories.
The build packages the AppImage without FUSE and skips the AppImageLint compatibility check, which only runs when you package QGC outside a container.
Environment variables adjust the run:

| Variable      | Purpose                                                     |
| ------------- | ----------------------------------------------------------- |
| `BUILD_DIR`   | Build output directory (default: `build` in the source)     |
| `SOURCE_DIR`  | Source directory for `run` (default: the current directory) |
| `JOBS`        | Number of parallel build jobs                               |
| `CLEAN_BUILD` | `1` empties the build directory first (`aarch64` variant)   |

Depending on your system resources, or the resources assigned to your Docker Daemon, the build step can take some time.

## Building Manually

If you want to build using the container manually, first build the image by running the following command from the root of the QGC source code directory:

```sh
docker build --target linux --file ./deploy/docker/Dockerfile -t qgc-ubuntu-docker .
```

::: info
The `-t` flag is essential.
Keep in mind this is tagging the image for later reference since you can have multiple builds of the same container.
:::

::: info
The `linux` image builds natively for the host's architecture (x86_64 or arm64).
The `linux-cross` and `android` targets and the `arch` variant need an x86_64 image, so on an arm64 host, such as an Apple silicon Mac, build them with `--platform linux/amd64`, which Docker emulates.
:::

Then create a `build` directory in the source tree to hold the artifacts, and run the image using the tag provided above, from the root directory:

```sh
mkdir build
docker run --rm -v ${PWD}:/project/source -v ${PWD}/build:/project/build qgc-ubuntu-docker
```

On a Windows host, reference the current directory with `%cd%` instead:

```sh
docker run --rm -v %cd%:/project/source -v %cd%/build:/project/build qgc-ubuntu-docker
```

::: info
These commands run the build as `root`; the script runs it as your user instead.
See [`run_docker.py`](https://github.com/mavlink/qgroundcontrol/blob/master/deploy/docker/run_docker.py) for the exact `docker run` options the script uses.
:::

## VS Code Dev Container

[`.devcontainer/devcontainer.json`](https://github.com/mavlink/qgroundcontrol/blob/master/.devcontainer/devcontainer.json) builds the `devcontainer` target: the `linux` image with Qt and all dependencies preinstalled, the static-analysis tools that CI uses (LLVM, clang-tidy and Clazy, for `just analyze`), plus a non-root `vscode` user.
It builds natively on x86_64 and arm64 hosts, such as Apple Silicon Macs, installing the matching Qt.
It also has the ccache release CI pins, and on x86_64 the Android SDK, NDK, JDK and a Qt kit for each Android ABI: `qgc-android <abi> <command>` runs a command with that ABI's kit selected, for example `qgc-android arm64-v8a cmake --preset Android`.
Each Android preset uses one build directory for every ABI, so add `--fresh` to the configure command when you switch to another ABI.
Google publishes the NDK for x86_64 Linux hosts only, so on arm64 run the x86_64 image (`--platform linux/amd64`) for Android builds.
In VS Code, add `"options": ["--platform=linux/amd64"]` to `build` and `"runArgs": ["--platform=linux/amd64"]` in your local `devcontainer.json`.
Open the repository in VS Code with the [Dev Containers](https://code.visualstudio.com/docs/devcontainers/containers) extension installed and choose **Reopen in Container**.
The container installs the C++, CMake, QML and Python extensions and the QGC workspace settings, tasks and launch configurations, so you can build, test and debug from VS Code.
CI publishes the image for both architectures as `ghcr.io/mavlink/qgc-dev` (`latest` from `master`, and each release's version tag), and the Dev Container uses it as a build cache.
Docker reuses the published layers only while your checkout's files match CI's, including their permissions, so a checkout made with a umask other than `022` rebuilds every layer.
You can also run the image directly, for example `docker run --rm -it -v "$PWD:/workspaces/qgroundcontrol" ghcr.io/mavlink/qgc-dev:latest`.
It runs as the `vscode` user (UID 1000), so that user needs write access to the mounted checkout.

## Troubleshooting

### Windows: 'bash\r': No such file or directory

This error indicates that a Linux script is being run with Windows line endings.
The repository's `.gitattributes` checks out shell scripts with Linux line endings, so this usually means the working tree was created by an older checkout or converted by another tool.

Recreate the working tree so Git applies the line-ending rules again (this discards uncommitted changes):

```sh
git rm --cached -r .
git reset --hard
```
