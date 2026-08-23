# 命令行选项

您可以使用命令行选项启动QGroundControl。 这些用于启用日志记录，运行单元测试以及模拟不同的主机环境以进行测试。 These are used to enable logging, run unit tests, and simulate different host environments for testing.

## 使用选项启动QGroundControl

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

Linux终端：

```sh
./QGroundControl-<arch>.AppImage --logging:full --log-output
```

## 选项

选项/命令行参数列在下表中。

| 选项                                                        | 描述                                                                                                                                                                |
| --------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `--clear-settings`                                        | Clears the app settings (reverts _QGroundControl_ back to default settings).                                                   |
| `--logging:full`                                          | Turns on full logging. See [Console Logging](../qgc-user-guide/troubleshooting/console_logging.md#logging-from-the-command-line). |
| `--logging:Comms.LinkManager,FactSystem.ParameterManager` | Turns on the specified comma separated logging categories.                                                                                        |
| `--log-output`                                            | Writes log messages to stderr (the terminal on macOS/Linux).                                                                   |
| `--unittest:name`                                         | （仅限调试版本）连续运行指定的单元测试次。 Leave off `:name` to run all tests.                                                                                         |
| `--unittest-stress:name`                                  | （仅限调试版本）连续运行指定的单元测试20次。 Leave off :name to run all tests.                                                                         |
| `--fake-mobile`                                           | Simulates running on a mobile device.                                                                                                             |
| `--test-high-dpi`                                         | Simulates running _QGroundControl_ on a high DPI device.                                                                                          |

笔记：

- 单元测试自动包含在调试版本中（作为QGroundControl的一部分）。 QGroundControl在单元测试的控制下运行（它不能正常启动）。 | `
  clear-settings` | 清除应用程序设置（将QGroundControl恢复为默认设置）。 |
  \| `logging:full` | 打开完整日志记录。 请参阅控制台日志记录 |
  \| `logging:full,LinkManagerVerboseLog,ParameterLoaderLog` | 打开完整日志记录并关闭以下列出的以逗号分隔的日志记录选项。 |
  \| `--llogging:LinkManagerLog,ParameterLoaderLog` | 打开指定的逗号分隔日志记录选项 |
  \| `--unittest:name` | （仅限Debug构建）运行指定单元测试。 离开：运行所有测试的名称。 |
  \| `--unittest-stress:name` | （仅限调试版本）连续运行指定的单元测试20次。 离开：运行所有测试的名称。 |
  \| `-fake-mobile` | 模拟在移动设备上运行。 |
  \| `--test-high-dpi` | 模拟在高DPI设备上运行QGroundControl。 |
