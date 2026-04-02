# plan

- intake:
  - hit1: `docs/build-log.md:824-846` `FAILED: _deps/mavlink-build/include/mavlink/all/mavlink.h`
  - hit2: `src/MAVLink/CMakeLists.txt:33-58` `CPMAddPackage(NAME mavlink ...)`
  - hit3: `build/.../cpm_modules/mavlink/.../CMakeLists.txt:30-41` `add_custom_command(... pip install -r pymavlink/requirements.txt ... mavgen ...)`
- examples:
  - `custom-example/CMakeLists.txt`
  - `custom-example/cmake/CustomOverrides.cmake`
- affected_files:
  - `src/MAVLink/CMakeLists.txt:31-58 ~+20`
  - `custom/docs/current/overview.md:1-11 ~+11`
  - `custom/docs/current/plan.md:1-24 ~+24`
  - `custom/docs/current/task.md:1-40 ~+40`
- implementation:
  1. 在 `CPMAddPackage` 完成后定位 `${mavlink_SOURCE_DIR}/pymavlink/requirements.txt`。
  2. 若文件存在，读取内容并删除开发依赖段中的 `pytest`、`syrupy`、`wsproto`，回写原文件。
  3. 保留 `fastcrc/lxml/setuptools/wheel`，不修改 `mavgen` 输出与 include 目录。
  4. 通过 `view` 与 `diagnostics` 做静态复核。
- checkpoints:
  - `requirements.txt` patch 仅发生在 `mavlink_ADDED` 分支
  - 不新增跨层依赖
  - 不修改 `custom/res/USVPayloadPanel.qml`
- verify_commands:
  - `cmake --build <build_dir> --target mavlink`
  - `cmake --build <build_dir> --target all`
- rollback:
  - `git restore --source=HEAD -- src/MAVLink/CMakeLists.txt custom/docs/current/*.md`

