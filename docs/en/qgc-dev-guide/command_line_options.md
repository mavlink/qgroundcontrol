# Command Line Options

You can start _QGroundControl_ with command line options. These are used to enable logging, run unit tests, and simulate different host environments for testing.

## Starting QGroundControl with Options

You will need to open a command prompt or terminal, change directory to where **qgroundcontrol.exe** is stored, and then run it. This is shown below for each platform (using the `--logging:full` option):

Windows Command Prompt:

```sh
cd "\Program Files (x86)\qgroundcontrol"
qgroundcontrol --logging:full
```

OSX Terminal app (**Applications/Utilities**):

```sh
cd /Applications/qgroundcontrol.app/Contents/MacOS/
./qgroundcontrol --logging:full
```

Linux Terminal:

```sh
./qgroundcontrol-start.sh --logging:full
```

## Options

The options/command line arguments are listed in the table below.

| Option                                                    | Description                                                                                                                          |
| --------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------ |
| `--clear-settings`                                        | Clears the app settings (reverts _QGroundControl_ back to default settings).                                                         |
| `--logging:full`                                          | Turns on full logging. See [Console Logging](../../qgc-user-guide/settings_view/console_logging.html#logging-from-the-command-line). |
| `--logging:full,LinkManagerVerboseLog,ParameterLoaderLog` | Turns on full logging and turns off the following listed comma-separated logging options.                                            |
| `--logging:LinkManagerLog,ParameterLoaderLog`             | Turns on the specified comma separated logging options                                                                               |
| `--unittest:name`                                         | (Debug builds only) Runs the specified unit test. Leave off `:name` to run all tests.                                                |
| `--unittest-stress:name`                                  | (Debug builds only) Runs the specified unit test 20 times in a row. Leave off :name to run all tests.                                |
| `--fake-mobile`                                           | Simulates running on a mobile device.                                                                                                |
| `--test-high-dpi`                                         | Simulates running _QGroundControl_ on a high DPI device.                                                                             |

Notes:

- Unit tests are included in debug builds automatically (as part of _QGroundControl_). _QGroundControl_ runs under the control of the unit test (it does not start normally).
