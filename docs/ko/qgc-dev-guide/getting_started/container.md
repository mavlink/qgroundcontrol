# Building using Containers

The community created a docker image that makes it much easier to build a Linux-based QGC application.
This can give you a massive boost in productivity and help with testing.

## About the Container

The Container is located in the `./deploy/docker` directory.
It's based on ubuntu 20.04.
It pre-installs all the dependencies at build time, including Qt, thanks to a script located in the same directory, `install-qt-linux.sh`.
The main advantage of using the container is the usage of the `CMake` build system and its many improvements over `qmake`

## Building the Container

### Script

To build the container using the script, run this command in the qgc root directory

```
./deploy/docker/run-docker-ubuntu.sh
```

### Manual

if you want to Build using the container manually, then you first have to build the image.
You can accomplish this using docker, running the following script from the root of the QGC source code directory.

```
docker build --file ./deploy/docker/Dockerfile-build-ubuntu -t qgc-ubuntu-docker .
```

:::info
The `-t` flag is essential.
Keep in mind this is tagging the image for later reference since you can have multiple builds of the same container
:::

::: info
If building on a Mac computer with an M1 chip you must also specify the build option `--platform linux/x86_64` as shown:

```
docker build --platform linux/x86_64 --file ./deploy/docker/Dockerfile-build-ubuntu -t qgc-ubuntu-docker .
```

Otherwise you will get a build error like:

```
qemu-x86_64: Could not open '/lib64/ld-linux-x86-64.so.2': No such file or directory
```

:::

## Building QGC using the Container

To use the container to build QGC, you first need to define a directory to save the artifacts.
We recommend you create a `build` directory on the source tree and then run the docker image using the tag provided above as follows, from the root directory:

```
mkdir build
docker run --rm -v ${PWD}:/project/source -v ${PWD}/build:/project/build qgc-ubuntu-docker
```

::: info
For up to date docker command and options reference relevant run-script in `deploy/docker`, for example [run-docker-ubuntu.sh](https://github.com/mavlink/qgroundcontrol/blob/master/deploy/docker/run-docker-ubuntu.sh#L16).

:::

::: info
If using the script to build the Linux image on a Windows host, you would need to reference the PWD differently.
On Windows the docker command is:

```
docker run --rm -v %cd%:/project/source -v %cd%/build:/project/build qgc-ubuntu-docker
```

:::

Depending on your system resources, or the resources assigned to your Docker Daemon, the build step can take some time.

## Troubleshooting

### Windows: 'bash\r': No such file or directory

This error indicates that a Linux script is being run with Windows line endings, which might occur if `git` was configured to use Windows line endings:

```
 > [4/7] RUN /tmp/qt/install-qt-linux.sh:
#9 0.445 /usr/bin/env: 'bash\r': No such file or directory
```

One fix is to force Linux line endings using the command:

```
git config --global core.autocrlf false
```

Then update/recreate your local repository.
