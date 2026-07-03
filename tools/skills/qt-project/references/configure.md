# Configure & Build Project

Covers trying out the project by locating framework and tool requirements,
configuring and building the project.

## Locating Requirements

**Locate cmake and ninja-build** tools so they can be called from command line by using the
commands `cmake` and `ninja`.

**Locate the installed Qt SDK** by determining the host OS first and then searching for typical
locations of the framework on that specific OS.
Keep in mind that in case of development environments, a local Qt installation can reside in the
user's folder instead of being installed on the system.
The goal is to find the framework's installation folder which contains toolchain commands in the
`bin` folder and under it, locate `bin/qt-cmake` command. Use this `qt-cmake` command below for
all configuration activities.
If not found or unsure, ask the user.

## Configuring the project for the first time

We prefer doing out-of-tree builds where the build folder is separate outside of the project folder.
In case of the sample project located in `MyApp` folder, on Unix-like platforms this looks like:

```bash
mkdir MyApp-build
cd MyApp-build
qt-cmake -S ../MyApp -B . -G Ninja
ninja
```

## Building from the command line

```bash
cd MyApp-build
ninja
```
