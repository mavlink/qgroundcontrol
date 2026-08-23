# Command Line Options

You can start _QGroundControl_ with command line options. These are used to enable logging, run unit tests, and simulate different host environments for testing.

## Starting QGroundControl with Options

You will need to open a command prompt or terminal, change directory to where the _QGroundControl_ executable is stored, and then run it. This is shown below for each platform (using the `--logging:full --log-output` options):

Windows Command Prompt (_QGroundControl_ is a GUI application with no console window, so redirect stderr to a file to capture the output):

```sh
cd "%ProgramFiles%\QGroundControl\bin"
QGroundControl --logging:full --log-output > "%USERPROFILE%\qgc_log.txt" 2>&1
```

macOS Terminal app (**Applications/Utilities**):

```sh
cd /Applications/QGroundControl.app/Contents/MacOS
./QGroundControl --logging:full --log-output
```

Linux Terminal:

```sh
./QGroundControl-<arch>.AppImage --logging:full --log-output
```

## Options

The options/command line arguments are listed in the table below.

| Option                                                    | Description                                                                                                                                                              |
| --------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `--clear-settings`                                        | Clears the app settings (reverts _QGroundControl_ back to default settings).                                                          |
| `--logging:full`                                          | Turns on full logging. See [Console Logging](../qgc-user-guide/troubleshooting/console_logging.md#logging-from-the-command-line).        |
| `--logging:Comms.LinkManager,FactSystem.ParameterManager` | Turns on the specified comma separated logging categories.                                                                                               |
| `--log-output`                                            | Writes log messages to stderr (the terminal on macOS/Linux).                                                                          |
| `--unittest:name`                                         | (Debug builds only) Runs the specified unit test. Leave off `:name` to run all tests.                                 |
| `--unittest-stress:name`                                  | (Debug builds only) Runs the specified unit test 20 times in a row. Leave off :name to run all tests. |
| `--fake-mobile`                                           | Simulates running on a mobile device.                                                                                                                    |
| `--test-high-dpi`                                         | Simulates running _QGroundControl_ on a high DPI device.                                                                                                 |

Notes:

- Unit tests are included in debug builds automatically (as part of _QGroundControl_). _QGroundControl_ runs under the control of the unit test (it does not start normally).
