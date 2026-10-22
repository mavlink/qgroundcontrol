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

Options take their value either after a colon (`--logging:full`) or as the next argument (`--logging full`).
Run with `--help` to list the options available in your build.

## Options

The options/command line arguments are listed in the table below.

| Option                                                    | Description                                                                                                                       |
| --------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| `--help`, `--version`                                     | Prints the available options or the version, then exits.                                                                          |
| `--clear-settings`                                        | Clears the app settings (reverts _QGroundControl_ back to default settings).                                                      |
| `--clear-cache`                                           | Clears the cached parameter and airframe data.                                                                                    |
| `--logging:full`                                          | Turns on full logging. See [Console Logging](../qgc-user-guide/troubleshooting/console_logging.md#logging-from-the-command-line). |
| `--logging:Comms.LinkManager,FactSystem.ParameterManager` | Turns on the specified comma separated logging categories. A category prefix such as `Comms` also turns on its sub-categories.    |
| `--log-output`                                            | Writes log messages to stderr (the terminal on macOS/Linux).                                                                      |
| `--system-id:id`                                          | Sets the MAVLink system ID used by _QGroundControl_.                                                                              |
| `--simple-boot-test`                                      | Initializes the application subsystems and exits (a quick startup check).                                                         |
| `--fake-mobile`                                           | (Desktop only) Simulates running on a mobile device.                                                                              |
| `--allow-multiple`                                        | (Desktop only) Allows more than one instance of _QGroundControl_ to run at a time.                                                |
| `--swrast`                                                | (Windows and macOS only) Forces software rendering.                                                                               |
| `--no-windows-assert-ui`                                  | (Windows only) Disables the assert dialog boxes.                                                                                  |

### Unit Test Options

These options are only available in builds with unit tests (`QGC_BUILD_TESTING`, on by default in Debug builds). See [Unit Tests](contribute/unit_tests.md).

| Option                         | Description                                                                                      |
| ------------------------------ | ------------------------------------------------------------------------------------------------ |
| `--unittest:name`              | Runs the specified unit test. Leave off `:name` to run all tests.                                |
| `--unittest-stress:count`      | Repeats unit tests `count` times (`count > 0`). Combine with `--unittest:name` to select a test. |
| `--unittest-output:file`       | Writes the results to `file` in JUnit XML format.                                                |
| `--unittest-shard:index/count` | Runs every `count`-th test function of a single test, starting at `index` (1-based).             |
| `--label:labels`               | Selects tests by comma separated labels, for example `--label:Unit,Vehicle`.                     |
| `--list-tests`                 | Lists the registered tests (combine with `--label` to filter) and exits.                         |
| `--onscreen`                   | Shows test windows on screen instead of running offscreen.                                       |

Notes:

- For example, `--unittest:ParameterManagerTest --unittest-stress:20` runs `ParameterManagerTest` 20 times.
- When running unit tests, _QGroundControl_ runs under the control of the unit test (it does not start normally).
