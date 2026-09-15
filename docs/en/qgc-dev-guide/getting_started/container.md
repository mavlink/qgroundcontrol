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

::: info
If building on a Mac computer with an M1 chip you must also specify the build option `--platform linux/x86_64` as shown:

```sh
docker build --platform linux/x86_64 --target linux --file ./deploy/docker/Dockerfile -t qgc-ubuntu-docker .
```

Otherwise you will get a build error like:

```sh
qemu-x86_64: Could not open '/lib64/ld-linux-x86-64.so.2': No such file or directory
```

:::

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

QGC has one default development container: the `devcontainer` target in
`deploy/docker/Dockerfile`. Both `.devcontainer/devcontainer.json` and image publication
build that same target and `linux/amd64` platform. It includes Clang,
clang-tidy, clang-scan-deps, clangd, and Clazy built against the same LLVM.
`.github/build-config.json` supplies the LLVM major version, checksum-verified Clazy
revision, and Qt version used by CI. The image build uses `deploy/docker/install_analysis.py`;
ccache uses the pinned, signature-verified release from `.github/scripts/ccache_helper.py`.
Existing application builder images are unchanged.

The dedicated QGC Development Image workflow (`devcontainer.yml`) publishes this same
default container in its own build pipeline:

- Upstream master pushes that change image inputs publish `ghcr.io/mavlink/qgroundcontrol-dev:latest`.
  Inputs include the Dockerfile, copied installers/helpers, shared configuration, Python locks,
  default devcontainer configuration, and publication workflow. Source-only, test-only, and
  documentation-only changes outside copied image inputs do not rebuild it.
  New input-changing pushes cancel older builds; unrelated master commits do not suppress them.
- Published stable QGC releases publish `ghcr.io/mavlink/qgroundcontrol-dev:<QGC-release-tag>`
  from the released tag's source and configuration, not current master.
  For example, release `v5.1.4` maps to `ghcr.io/mavlink/qgroundcontrol-dev:v5.1.4`.
  Drafts and prereleases are excluded. Stable publication never updates `latest`.

The release workflow dispatches the same image pipeline for releases created with
`GITHUB_TOKEN`, which do not generate downstream release events.
The publication summary provides
`ghcr.io/mavlink/qgroundcontrol-dev@sha256:<digest>` for immutable pinning.

Relevant pull requests and ordinary manual dispatches validate without publishing.
To publish or retry an existing stable release's image, manually dispatch with `release_tag`;
the workflow verifies that it is a published, non-prerelease QGC release before building.
There is no separate container version counter or Docker Hub mirror.
No application or analysis workflows consume it yet;
future analysis jobs can pin this same development image instead of maintaining
a separate environment.
The image builds only tooling, not QGC; Qt, Python dependencies, LLVM, and Clazy are
already installed, with no package installation or downloads at container startup.

Qt, Python, and analysis executables are on `PATH` for non-login shells and non-root users.
Image builds check tool versions, Clazy's LLVM linkage, and compiler startup.
To build and open a development shell locally:

```sh
docker build --platform linux/amd64 --target devcontainer \
  -f deploy/docker/Dockerfile -t qgc-devcontainer .
docker run --rm -it -v "$PWD:/workspaces/qgroundcontrol" qgc-devcontainer
```

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
