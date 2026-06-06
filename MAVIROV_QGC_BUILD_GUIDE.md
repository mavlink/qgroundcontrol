# MIST MAVIROV — Custom QGroundControl Build Guide

This source tree is stock QGroundControl (master, commit `d31c2ae0`) plus a
**MAVIROV ROV Companion** module. Everything else in QGC is untouched — flying,
setup, parameters, missions, joystick, video and all other features behave
exactly like the official QGC.

## What was added

A collapsible **MAVIROV** tab on the right edge of the Fly view. Open it and you get:

* **▶ CONNECT ALL** — one tap does the whole bring-up, exactly like ROV GCS v3.4:
  1. Waits up to 90 s for the Pi at `192.168.2.2:22` to finish booting (probes every 2 s).
  2. SSH channel 1 → `pi@192.168.2.2` (password `pi`):
     `source ~/mavenv/bin/activate && mavproxy.py --master=/dev/ttyACM0 --baudrate 115200 --out=udp:<PC>:14550 --out=udp:<PC>:14551`
  3. SSH channel 2 → `farhanpi@192.168.2.2` (password `pi`): `python3 ~/multi_camera_rtsp.py`
  4. QGC's normal UDP autoconnect picks up the Pixhawk on **14550** by itself —
     no link setup needed. Port **14551** stays free for the Python ROV GCS, so
     both can run at the same time.
  5. QGC's video switches to the selected RTSP camera automatically.
  Host-key prompts are auto-accepted. If anything fails the button shows
  RETRY; if the cameras fail, MAVLink still comes up (same as the Python app).
  **■ DISCONNECT ALL** closes both SSH channels, which kills MAVProxy and the
  camera script on the Pi (the channels run with a pty, like the Python app).
* **CAM 1 / CAM 2 / CAM 3** — switches QGC's video to
  `rtsp://192.168.2.2:8554/cam0`, `:8555/cam1`, `:8556/cam2`.
* **Robotic arm** — the same 3-D arm visualiser as the Python app (ported 1:1),
  live claw cm / roll ° readouts, ROLL ◀ ▶ / OPEN / CLOSE / CENTER / SYNC
  buttons, and UDP `PULSE1:<µs>` / `PULSE2:<µs>` datagrams to the Teensy at
  `192.168.2.1:8888`. Servo 1 = claw (500 closed → 1860 open, 12 cm max),
  servo 2 = roll (500–2500, 1525 = 0°).
* **Keyboard servo control** checkbox — same bindings as the Python app:
  `W/S` fast claw close/open, `A/D` fast roll, `↑/↓` fine claw, `←/→` fine
  roll (big step 15 µs, small step 3 µs).
* **Link log** — timestamps for the probe, SSH output and servo events.

### Files added / changed

```
src/ROVCompanion/ROVCompanionController.h    (new — C++ QML singleton)
src/ROVCompanion/ROVCompanionController.cc   (new)
src/ROVCompanion/CMakeLists.txt              (new)
src/FlyView/ROVCompanionPanel.qml            (new — the drawer UI)
src/FlyView/ROVArmView.qml                   (new — 3-D arm canvas)
src/CMakeLists.txt                           (1 line: add_subdirectory)
src/FlyView/CMakeLists.txt                   (2 lines: register QML files)
src/FlyView/FlyViewWidgetLayer.qml           (8 lines: place the panel)
.github/build-config.json                    (restored — see note below)
```

`mavirov-stock-file-changes.patch` contains the diff of the three stock files.

> **Note:** the zip you sent was made with GitHub's "Download ZIP", which
> strips `.github/` (export-ignore). Without `.github/build-config.json` the
> stock CMake aborts immediately, so that file was restored from the same
> commit. GitHub workflow files are stripped the same way, which matters for
> the CI route below.

## Building

QGC master requires **Qt 6.10.3** (minimum 6.10.0) and **CMake ≥ 3.25**, so it
must be built on your machine (or GitHub CI) with the official Qt kit.

### Windows 10/11 (x64)

1. Install **Visual Studio 2022 Community** with the "Desktop development with C++" workload.
2. Install **CMake ≥ 3.25** (or use the one bundled with VS) and **Git**.
3. Run the **Qt Online Installer**, sign in, and under *Qt 6.10.3 → MSVC 2022 64-bit*
   also tick these additional libraries (they match `.github/build-config.json`):
   Qt Graphs, Qt Location, Qt Positioning, Qt Speech, Qt Multimedia,
   Qt Serial Port, Qt Image Formats, Qt Shader Tools, Qt Connectivity,
   Qt Quick 3D, Qt Sensors, Qt State Machines (SCXML), Qt WebSockets,
   Qt HTTP Server. Under *Build Tools*, include Ninja and CMake if you want
   Qt's copies.
4. Open **"x64 Native Tools Command Prompt for VS 2022"** in the source folder and run:

   ```bat
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
         -DCMAKE_PREFIX_PATH=C:\Qt\6.10.3\msvc2022_64
   cmake --build build --config Release --target QGroundControl
   ```

   The app lands in `build\Release\QGroundControl.exe`. To run it outside the
   build tree, run `C:\Qt\6.10.3\msvc2022_64\bin\windeployqt.exe` on the exe,
   or build the `package`/installer target the official docs describe.
5. **SSH on Windows:** password logins need PuTTY's `plink.exe` — put it next
   to `QGroundControl.exe` or in `PATH` (the module finds it automatically and
   answers the host-key prompt). Without it, the module falls back to Windows
   OpenSSH, which only works with key authentication.

### Linux (Ubuntu 22.04+)

```bash
sudo bash ./tools/setup/install-dependencies-debian.sh   # GStreamer etc.
# Install Qt 6.10.3 (online installer or aqtinstall) with the same modules,
# e.g.: aqt install-qt linux desktop 6.10.3 linux_gcc_64 \
#       -m qtgraphs qtlocation qtpositioning qtspeech qtmultimedia qtserialport \
#          qtimageformats qtshadertools qtconnectivity qtquick3d qtsensors \
#          qtscxml qtwebsockets qthttpserver
~/Qt/6.10.3/gcc_64/bin/qt-cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target QGroundControl -j$(nproc)
./build/QGroundControl
```

For password SSH install `sshpass` (`sudo apt install sshpass`); with it the
module uses `sshpass + ssh -tt` exactly like the Python app's behaviour.

### Easiest route: let GitHub build the installer for you

Because "Download ZIP" strips the workflow files, CI builds need the real repo:

1. Fork `https://github.com/mavlink/qgroundcontrol` on GitHub.
2. `git clone --recursive` your fork and `git checkout d31c2ae09084913474d520ed4c8648d273a1f1ee`
   (the commit this source matches), then create a branch.
3. Copy in `src/ROVCompanion/`, `src/FlyView/ROVArmView.qml`,
   `src/FlyView/ROVCompanionPanel.qml`, and apply
   `mavirov-stock-file-changes.patch` (`git apply mavirov-stock-file-changes.patch`).
4. Commit and push. The repo's own GitHub Actions build Windows installer,
   Linux AppImage, macOS dmg and Android apk artifacts for you.

## Runtime configuration — `ROVCompanion.json`

On first start the module writes `ROVCompanion.json` with defaults **next to
the QGroundControl executable** (or in the app-config folder if that's not
writable — the exact path is printed at the top of the Link Log). Edit it to
change anything; every value mirrors `rov_gcs_config.json`:

```json
{
    "pi_address": "192.168.2.2",
    "pc_address": "",                 // empty = auto-detect on the Pi's subnet, fallback 192.168.2.3
    "qgc_mavlink_port": 14550,
    "extra_mavlink_port": 14551,
    "mavlink_ssh_user": "pi",
    "mavlink_ssh_password": "pi",
    "camera_ssh_user": "farhanpi",
    "camera_ssh_password": "pi",
    "ssh_program": "",                // optional explicit path to ssh/plink
    "pi_boot_timeout_s": 90,
    "pi_probe_interval_s": 2.0,
    "mavproxy_command": "source ~/mavenv/bin/activate && mavproxy.py --master=/dev/ttyACM0 --baudrate 115200 --out=udp:{PC_IP}:{QGC_MAVLINK_PORT} --out=udp:{PC_IP}:{MAVLINK_PORT}",
    "camera_command": "python3 ~/multi_camera_rtsp.py",
    "rtsp_base_port": 8554,
    "camera_count": 3,
    "teensy_address": "192.168.2.1",
    "teensy_port": 8888,
    "claw_min_pulse": 500,
    "claw_max_pulse": 1860,
    "roll_min_pulse": 500,
    "roll_max_pulse": 2500,
    "roll_center_pulse": 1525,
    "claw_max_cm": 12.0,
    "servo_big_step": 15,
    "servo_small_step": 3
}
```

`{PC_IP}`, `{QGC_MAVLINK_PORT}` and `{MAVLINK_PORT}` in `mavproxy_command` are
replaced automatically.

## Quick use

1. Plug in the tether, start QGroundControl.
2. Fly view → tap the **MAVIROV** tab on the right edge → **▶ CONNECT ALL**.
3. Watch the log: *Waiting for Pi → MAVProxy → Cameras → CONNECTED*. The
   Pixhawk appears as a normal QGC vehicle and the video shows CAM 1.
4. Switch cameras with CAM 1/2/3; drive the claw/roll with the buttons or tick
   *Keyboard servo control* for WASD/arrows.
5. **■ DISCONNECT ALL** shuts everything down.
