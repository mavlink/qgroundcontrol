# Android serial ports

QGC's `QSerialPort` and `QSerialPortInfo` for Android. Qt Serial Port opens device nodes only, and Android
lets an app reach a USB serial adapter only through the USB host API of its Java side. So QGC has classes of
its own under Qt's names, and the rest of QGC includes `<QtSerialPort/QSerialPort>` on every platform.

A port is one of two kinds, which its name alone tells:

- A **USB port**, which Java drives. Its name is the one that Android gives the device (`/dev/bus/usb/...`),
  or the stable name that survives replugging (`usb:...`).
- A **serial device node**, such as `/dev/ttyS1`, which the POSIX backend opens. The port list has the nodes
  only while the "Use built-in serial ports" setting is on, and only those that the device lets an app open.
  The node of a USB device that Java lists a port of is left out, so that no device is listed twice.

## Layers

| Layer | Files | What it is |
| --- | --- | --- |
| Public headers | `include/` | All that the app may use: the two `QtSerialPort` headers, `AndroidSerialAccess.h` (which ports are listed, and the USB permission) and `AndroidSerialApp.h` (the registration of Java's callbacks, the boot self-test and the device watcher). |
| Port | `AndroidSerialPort.*`, `AndroidSerialPortInfo.*`, `AndroidSerialAccess.cc` | `QSerialPort` on the public interface of `QIODevice`, with its buffers, waits and errors, and `QSerialPortInfo` with the port list. |
| Backends | `AndroidSerialBackend.h`, `UsbSerialBackend.cc`, `PosixSerialBackend.cc`, `PosixSerialPorts.cc` | One interface for the two kinds of port. A backend reaches its port through `AndroidSerialBackendHost` only. |
| JNI | `AndroidSerialJni.*`, `JavaBinding.h`, `AndroidSerialRegistry.cc`, `AndroidSerialSelfTest.cc` | The calls into Java and the callbacks from it, in the namespace `AndroidSerial`. A callback finds its port in the registry, by the handle that C++ gave the port when it opened it. |
| Java | `android/src/org/mavlink/qgroundcontrol/serial` | The USB side. Its `package-info.java` describes the classes, their threads and their locks. |

Everything but `include/` is private to the library and its tests.

Qt installs its own Serial Port headers as well, whose classes differ in size from these. The root
`CMakeLists.txt` therefore puts `include/` ahead of Qt's headers for every target of an Android build.

## Tests

| What is tested | Where | How it runs |
| --- | --- | --- |
| The port and both backends, built for a Linux host | `test/Android/Serial` | `ctest --test-dir build -R AndroidSerialPort` |
| The Java side | `android/src/test/java/org/mavlink/qgroundcontrol/serial` | `./gradlew testDebugUnitTest` in the `android-build` directory of an Android build |
| That C++ and Java declare the same JNI calls | `tools/tests/ci/contracts/test_android_serial_jni_contract.py` | `just test-python` |
| That C++ and Java reach each other in the packaged app | `AndroidSerial::selfTest()` | QGC's boot test (`--simple-boot-test`), which CI runs in an emulator |

The host tests compile this directory under test-only names, beside Qt's own `QSerialPort`. Only the Java side
and the JNI calls to it are simulated (`AndroidSerialFake`); the POSIX backend opens pseudo-terminals.

## Logs

The logging categories, and the byte-level trace of a port, are described for users under
[Android USB Serial Logging](../../../docs/en/qgc-user-guide/troubleshooting/console_logging.md#android-usb-serial-logging).
