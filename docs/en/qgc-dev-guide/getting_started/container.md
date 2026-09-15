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
satisfying the configured minimum; mobile and Apple SDK versions do not apply here.
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

To build and load the host platform using the same Bake target as CI, run from the
repository root (use `linux/amd64` on an x86-64 Docker host):

```sh
docker buildx bake -f deploy/docker/docker-bake.hcl qgc-dev \
  --set qgc-dev.platform=linux/arm64 --load
docker run --rm -it -v "$PWD:/workspaces/qgroundcontrol" qgc-dev:local
```

For a full native Linux QGC build, initialize the checkout's submodules, make its
build directory writable by the container user, and run the existing recipes:

```sh
git submodule update --init --recursive
mkdir -p build
docker run --rm -it -v "$PWD:/workspaces/qgroundcontrol" qgc-dev:local \
  bash -c 'git config --global --add safe.directory /workspaces/qgroundcontrol && just release'
```

On Linux, map ownership to the development user's UID/GID (1000 by default), or
set `USER_UID`/`USER_GID` when building the image. `just` uses detected CPU parallelism;
set `JOBS` to limit it. The result is `build/Release/QGroundControl`, a Linux artifact
even when Docker runs on macOS or Windows. Image publication runs a native amd64
QGC acceptance build in addition to bounded smoke checks on both architectures.

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
