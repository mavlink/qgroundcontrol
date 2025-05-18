# 单元测试

_QGroundControl_ (QGC) contains a set of unit tests that must pass before a pull request will be accepted. 向QGC添加新的复杂子系统应该有相应的新单元测试来测试它。

单元测试的完整列表可以在UnitTestList.cc中找到。

要运行单元测试：

1. 使用 `QGC_UNITTEST_BUILD` 定义在 `调试` 模式下构建。
2. 复制debug目录中的deploy / qgroundcontrol-start.sh脚本
3. 在命令行使用 `--unittst` 选项从命令行运行 _所有_ 单元测试。
   对于 Linux，操作方法如下：
   ```
   使用--unittest命令行选项从命令行运行所有单元测试。 对于Linux，这是完成如下所示： `qgroundcontrol-start.sh --unittest`
   ```
4. 也指定测试名称：`--unittest:RadioConfigTest`，单独运行 _单个_ 单元测试。
   对于 Linux，操作方法如下：
   ```
   通过指定测试名称来运行单个单元测试： - unittest：RadioConfigTest。 对于Linux，这是完成如下所示： `qgroundcontrol-start.sh --unittest:RadioConfigTest`
   ```
