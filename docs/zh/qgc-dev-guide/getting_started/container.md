# 使用容器构建

社区创建了一个 Docker 镜像，这使得构建基于 Linux 的 QGC 应用程序变得容易得多。
这可以大大提高您的生产力，并帮助您进行测试。

## 关于容器

容器位于 `./depl/docker` 目录中。
It's based on **Ubuntu 24.04**. However, always check the first lines of the Dockerfile in the aforementioned directory to have zero surprises.

It checks and pre-installs all the dependencies at build time, including Qt, thanks to the use of scripts from the directory `./tools/setup`.
The main advantage of using the container is the usage of the `CMake` build system and its many improvements over `qmake`.

## 构建容器

### 脚本

要使用脚本构建容器，请在 qgc 根目录中运行此命令

```sh
./depl/docker/run-docker-ubuntu.sh
```

### 手册

如果你想要手动使用容器构建，那么你必须先构建镜像。
您可以使用 docker 完成这个操作，从QGC 源代码目录的根目录运行下面的脚本。

```sh
docker build --file ./deploy/docker/Dockerfile-build-ubuntu -t qgc-ubuntu-docker .
```

:::info
`-t` 选项至关重要。
请记住，这是为镜像添加标签以便日后引用，因为同一容器可能会有多个构建版本。
:::

::: info
If building on a Mac computer with an M1 chip you must also specify the build option `--platform linux/x86_64` as shown:

```sh
docker build --platform linux/x86_64 --file ./deploy/docker/Dockerfile-build-ubuntu -t qgc-ubuntu-docker .
```

否则，你将会遇到一个构建错误，比如：

```sh
qemu-x86_64: Could not open '/lib64/ld-linux-x86-64.so.2': No such file or directory
```

:::

## 使用容器构建QGC

要使用该容器构建 QGC，首先需要定义一个目录来保存构建产物。
我们建议你在源代码树中创建一个“build”目录，然后从根目录使用上述提供的标签按如下方式运行Docker镜像：

```sh
mkdir build
docker run --rm -v ${PWD}:/project/source -v ${PWD}/build:/project/build qgc-ubuntu-docker
```

::: info
For up to date docker command and options reference relevant run-script in `deploy/docker`, for example [run-docker-ubuntu.sh](https://github.com/mavlink/qgroundcontrol/blob/master/deploy/docker/run-docker-ubuntu.sh#L16).

:::

::: info
If using the script to build the Linux image on a Windows host, you would need to reference the PWD differently.
On Windows the docker command is:

```sh
docker run --rm -v %cd%:/project/source -v %cd%/build:/project/build qgc-ubuntu-docker
```

:::

根据您的系统资源或分配给您的 Docker 守护进程的资源，构建步骤可能需要一些时间。

## 故障处理

### Windows: 'bash\r': No such file or directory

此错误表明正在以Windows换行符格式运行Linux脚本。如果将 `git` 配置为使用Windows换行符，就可能出现这种情况。

```sh
 > [4/7] RUN /tmp/qt/install-qt-linux.sh:
#9 0.445 /usr/bin/env: 'bash\r': No such file or directory
```

一个修复是强制使用 Linux 行尾使用命令：

```sh
git config --global core.autocrlf false
```

然后更新/重新创建您的本地资源库。 For, example, use:

```sh
git rm --cached -r .
git reset --hard
```
