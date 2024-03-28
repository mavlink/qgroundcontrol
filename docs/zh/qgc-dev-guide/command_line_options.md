# 命令行选项

您可以使用命令行选项启动QGroundControl。 这些用于启用日志记录，运行单元测试以及模拟不同的主机环境以进行测试。 These are used to enable logging, run unit tests, and simulate different host environments for testing.

## 使用选项启动QGroundControl

您需要打开命令提示符或终端，将目录更改为存储qgroundcontrol.exe的位置，然后运行它。 每个平台如下所示（使用--logging：full选项）： This is shown below for each platform (using the `--logging:full` option):

Windows命令提示符：

```sh
cd "\Program Files (x86)\qgroundcontrol"
qgroundcontrol --logging:full
```

OSX终端应用程序（应用程序/实用程序）：

```sh
cd /Applications/qgroundcontrol.app/Contents/MacOS/
./qgroundcontrol --logging:full
```

Linux终端：

```sh
./qgroundcontrol-start.sh --logging:full
```

## 选项

选项/命令行参数列在下表中。

| 选项                                                        | 描述                                                                                                                                   |
| --------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------ |
| `--clear-settings`                                        | Clears the app settings (reverts _QGroundControl_ back to default settings).                                      |
| `--logging:full`                                          | Turns on full logging. See [Console Logging](../../qgc-user-guide/settings_view/console_logging.html#logging-from-the-command-line). |
| `--logging:full,LinkManagerVerboseLog,ParameterLoaderLog` | Turns on full logging and turns off the following listed comma-separated logging options.                                            |
| `--logging:LinkManagerLog,ParameterLoaderLog`             | Turns on the specified comma separated logging options                                                                               |
| `--unittest:name`                                         | (Debug builds only) Runs the specified unit test. Leave off `:name` to run all tests.                             |
| `--unittest-stress:name`                                  | (Debug builds only) Runs the specified unit test 20 times in a row. Leave off :name to run all tests.             |
| `--fake-mobile`                                           | Simulates running on a mobile device.                                                                                                |
| `--test-high-dpi`                                         | Simulates running _QGroundControl_ on a high DPI device.                                                                             |

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
