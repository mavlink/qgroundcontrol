# overview

- module: `custom`, `src/MAVLink`
- objective: 修复 `docs/build-log.md` 中 `FAILED: _deps/mavlink-build/include/mavlink/all/mavlink.h` 对应的构建中断。
- root_cause: `build/.../cpm_modules/mavlink/.../CMakeLists.txt` 在生成 C 头文件前执行 `python -m pip install -r pymavlink/requirements.txt`，其中 `requirements.txt` 含开发依赖 `pytest`、`syrupy`、`wsproto`；日志显示 `syrupy` 访问 PyPI 失败导致 Ninja 中断。
- scope:
  - `src/MAVLink/CMakeLists.txt`
  - `custom/docs/current/overview.md`
  - `custom/docs/current/plan.md`
  - `custom/docs/current/task.md`
- kpi:
  - 移除构建期对 `syrupy` 的在线安装阻塞
  - 保持现有 `mavgen` 生成链与头文件包含路径不变
  - 不改动 QML/USV 业务逻辑
- rollback: `git restore --source=HEAD -- src/MAVLink/CMakeLists.txt custom/docs/current/overview.md custom/docs/current/plan.md custom/docs/current/task.md`

