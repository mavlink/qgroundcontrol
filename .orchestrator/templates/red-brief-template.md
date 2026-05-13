<task>
RED phase of slice {{N}}: write a failing test for {{behavior_description}}.

Test file: {{test_file_path}}
Module under test: {{module_path}} (does not yet exist or does not yet implement this behavior)

The test must fail at runtime (assertion, not build/link error in a way that
prevents the rest of the suite from running). Match the test file structure used by the
target subsystem — QtTest with `QTest::qExec`, `QCOMPARE`, `QVERIFY`, `QSignalSpy` /
`MultiSignalSpy`, derived from the project test base classes documented in
`test/TESTING.md`. For QML, use `TestCase` from `QtTest 1.x`.
</task>

<context>
Repo: this repository (qgroundcontrol — C++20 / Qt / QML).
Plan path: {{plan_path}}
Slice number: {{N}} (RED phase 1 of 2)
The next dispatch (GREEN) will implement the under-test code. Do not write that yet.

Constraints:
- DO NOT touch {{module_path}} in this dispatch. Only the test file.
- Test must fail when `{{verification_command}}` runs. `build/` must already be
  configured: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`.
- Match existing test style under `test/` and the patterns in `test/TESTING.md`.
  Register new tests under the appropriate CTest label (`Unit` or `Integration`).
- No `QSKIP`. The test must run and fail.
</context>

<structured_output_contract>
## summary
<1-2 lines>

## files_changed
- <test_file_path> — +<lines>

## tests_run
- command: `{{verification_command}}`
- result: 1 failed (<test name>) — <one-line failure reason>

## followups
- expected impl scope for GREEN phase: <hint>
</structured_output_contract>

<default_follow_through_policy>
- Test failure must come from the assertion or a clearly-scoped runtime error inside the
  test body, not from a build/link failure that breaks the whole suite.
- Match existing test class naming and QtTest patterns under `test/` (see `test/TESTING.md`).
</default_follow_through_policy>

<verification_loop>
1. Read existing tests under `test/` (and `test/TESTING.md`) to match style and CTest labels.
2. Write the failing test.
3. Run `{{verification_command}}` — confirm exactly one new failure with the right reason.
4. Return per output contract.
</verification_loop>
