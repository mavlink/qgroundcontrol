<task>
Execute Slice C-1 (preparation for the final audit log) of plan .orchestrator/work/phase1-slice16-strings-audit/plan.md: shadow the two upstream QML files that contain user-visible "QGroundControl" / "QGC" qsTr strings, so the Sprig GCS UI shows Sprig identity instead. After this slice, an `rg` audit for user-visible QGC/QGroundControl literals across `src/UI src/FlyView src/PlanView src/AnalyzeView src/QmlControls custom/` must return zero hits inside `qsTr(...)` content.

Files to create:
- custom/res/Custom/qml/QGroundControl/AppSettings/HelpSettings.qml — full shadow of src/UI/AppSettings/HelpSettings.qml with two qsTr strings rebranded
- custom/res/Custom/qml/QGroundControl/FlyView/PreFlightSoundCheck.qml — full shadow of src/FlyView/PreFlightSoundCheck.qml with two qsTr strings rebranded

Files to modify:
- custom/custom.qrc — register the two new shadow files under the existing `/Custom/qml` qresource prefix (lines 21–26)
</task>

<context>
Repo: qgroundcontrol Sprig GCS fork.
Plan path: .orchestrator/work/phase1-slice16-strings-audit/plan.md
Slice number: C-1 (refactor — Sprig identity replacement; no behavior change)

Background — the shadow mechanism:
- `SprigOverrideInterceptor::intercept` in `custom/src/SprigCorePlugin.cc:319-341` is a `QQmlAbstractUrlInterceptor`. For any QML URL with scheme `qrc` and path `/something`, it checks whether `:/Custom/something` exists; if so, it rewrites the URL to use the shadow path.
- Concretely: a request for `qrc:/qml/QGroundControl/Foo/Bar.qml` is rewritten to `qrc:/Custom/qml/QGroundControl/Foo/Bar.qml` IF the latter exists in the resource system. The shadow ships its own complete file.
- The existing precedent is `custom/res/Custom/qml/QGroundControl/Toolbar/SelectViewDropdown.qml`, registered in `custom/custom.qrc:25` as `<file alias="QGroundControl/Toolbar/SelectViewDropdown.qml">res/Custom/qml/QGroundControl/Toolbar/SelectViewDropdown.qml</file>` under `<qresource prefix="/Custom/qml">`.

Identifying the correct alias path for each upstream file:
- For each of `src/UI/AppSettings/HelpSettings.qml` and `src/FlyView/PreFlightSoundCheck.qml`, you must determine the QRC URL that QML normally resolves them under. Method:
  1. Find the file's `qt_add_qml_module` registration in its `src/.../CMakeLists.txt` (e.g., `src/UI/AppSettings/CMakeLists.txt:84` lists `HelpSettings.qml`).
  2. Identify the module URI (`URI` argument to `qt_add_qml_module`) and `RESOURCE_PREFIX` if any. Modern Qt6 qt_add_qml_module typically resolves under `qrc:/qt/qml/<URI as path>/<filename>` OR `qrc:/qml/<URI as path>/<filename>` depending on `RESOURCE_PREFIX`.
  3. Cross-check with the existing precedent: SelectViewDropdown upstream lives at `src/UI/toolbar/SelectViewDropdown.qml` and its shadow alias is `QGroundControl/Toolbar/SelectViewDropdown.qml` (note capital "Toolbar" — that's the module URI segment, not the filesystem dir). Find the actual module URI for HelpSettings and PreFlightSoundCheck the same way and use it.
  4. The shadow path under `custom/res/Custom/qml/` and the alias in `custom.qrc` must both match the resolved module path. **Both segments and casing must match exactly**, or the URL interceptor will not find the shadow at lookup time.

String replacements required:

In the **HelpSettings** shadow (copy upstream verbatim then change ONLY these two strings; do not touch the PX4/ArduPilot forum labels — those refer to upstream PX4/ArduPilot communities, not QGC identity):
- `qsTr("QGroundControl User Guide")` → `qsTr("Sprig GCS User Guide")`
- `qsTr("QGroundControl Discord Channel")` → `qsTr("Sprig Discord Channel")`
- Leave the `<a href="...">` URLs unchanged (replacing them with Sprig-specific URLs requires product input the user has not provided in this slice — preserving the URLs keeps the links functional; a follow-up slice will rotate URLs once Sprig docs/discord exist).

In the **PreFlightSoundCheck** shadow (copy upstream verbatim then change ONLY these two strings):
- `qsTr("QGC audio output enabled. System audio output enabled, too?")` → `qsTr("Sprig GCS audio output enabled. System audio output enabled, too?")`
- `qsTr("QGC audio output is disabled. Please enable it under application settings->general to hear audio warnings!")` → `qsTr("Sprig GCS audio output is disabled. Please enable it under application settings->general to hear audio warnings!")`

custom.qrc EDIT — CRITICAL: this file has documented merge fragility (sibling `<qresource>` blocks have been silently dropped on previous merges per project memory). You MUST:
1. Read the entire current `custom/custom.qrc` first and count the `<qresource prefix=` lines (currently 6: `/custom/img`, `/Custom/qmlimages`, `/Custom/qml`, `/Custom/res`, `/Custom/fonts`, `/res`).
2. Add the two new `<file alias="...">...</file>` entries INSIDE the existing `<qresource prefix="/Custom/qml">` block (around lines 21–26). Do NOT create a new `<qresource>` block.
3. After writing, re-read the file and verify the same 6 prefix blocks are still present with the same prefixes in the same order. If any sibling block disappeared, abort and restore from git.

Constraints:
- Operate in `custom/` only. Do not touch anything under `src/**`, root `CMakeLists.txt`, `cmake/**`, `.clang-format`, `.clang-tidy`, `.pre-commit-config.yaml`, `.github/**`, `docs/**`, `AGENTS.md`, `CODING_STYLE.md`.
- Do NOT modify the upstream `src/UI/AppSettings/HelpSettings.qml` or `src/FlyView/PreFlightSoundCheck.qml`.
- No new dependencies. No new `qt_add_qml_module` calls. The shadow mechanism is plain QRC file aliasing.
- The shadow files MUST be byte-for-byte identical to upstream except for the four `qsTr` string contents specified above. Do not "while we're here" reformat, reorder properties, change indentation, or alphabetize imports.
- `build/` is already configured at `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`.
- `UPSTREAM_PATCHES.md` must remain at zero active entries.
</context>

<structured_output_contract>
Return a single Markdown block:

## summary
<1-3 lines: shadows created, strings changed, qrc updated>

## files_changed
- custom/res/Custom/qml/QGroundControl/AppSettings/HelpSettings.qml — +N (created) — Sprig GCS rebrand of two help labels
- custom/res/Custom/qml/QGroundControl/FlyView/PreFlightSoundCheck.qml — +N (created) — Sprig GCS rebrand of pre-flight sound check prompts
- custom/custom.qrc — +2 — register the two new shadow aliases

## tests_run
- command: `cmake --build build --config Release --parallel && pre-commit run --files custom/custom.qrc custom/res/Custom/qml/QGroundControl/AppSettings/HelpSettings.qml custom/res/Custom/qml/QGroundControl/FlyView/PreFlightSoundCheck.qml`
- result: <build PASS/FAIL; pre-commit PASS/FAIL>

## audit_check
- command: `rg -i 'qsTr\(.*(qgroundcontrol|\bqgc\b)' --type qml src/UI src/FlyView src/PlanView src/AnalyzeView src/QmlControls custom/ | wc -l`
- result: <0 expected — report the number>
- Note: this check only counts hits inside qsTr() arguments. Module imports (`import QGroundControl`) and singleton refs (`QGroundControl.foo`) are not user-visible and remain.

## qrc_integrity_check
- After editing custom.qrc: `grep -c '<qresource prefix=' custom/custom.qrc`
- result: 6 (expected — no sibling block dropped)

## followups
- <any module-URI surprises, shadow lookup failures, or risks; empty if clean>
</structured_output_contract>

<default_follow_through_policy>
- Use `git mv` for renames — not applicable here.
- Confirm the shadow actually intercepts at runtime is not testable in this offline slice — qmllint + build + qrc integrity is the gate.
- If the upstream file's module URI cannot be determined unambiguously from its CMakeLists.txt (e.g., it's part of a multi-file plugin), STOP and report in followups. Do not guess a path.
- Stay inside the scoped files. Do not "while we're here" edit other QML files or fix unrelated qsTr strings.
- If `rg` post-audit reveals additional user-visible QGC/QGroundControl literals you didn't expect, list them in followups; do not auto-shadow more files in this slice.
</default_follow_through_policy>

<verification_loop>
1. Read `.orchestrator/work/phase1-slice16-strings-audit/plan.md` (just Slice C).
2. Read `src/UI/AppSettings/HelpSettings.qml` and `src/FlyView/PreFlightSoundCheck.qml` (the two upstream files to shadow).
3. Identify the QML module URI for each via its `src/.../CMakeLists.txt` `qt_add_qml_module` registration.
4. Read existing precedent: `custom/res/Custom/qml/QGroundControl/Toolbar/SelectViewDropdown.qml` and its line in `custom/custom.qrc:25` — match the structure.
5. Read entire current `custom/custom.qrc`. Note the 6 prefix blocks.
6. Create the two shadow files (full upstream content with only the 4 string changes specified above).
7. Edit `custom/custom.qrc` to add two `<file alias=...>` entries inside the `/Custom/qml` block.
8. Re-read `custom/custom.qrc` and verify all 6 prefix blocks remain. `grep -c '<qresource prefix=' custom/custom.qrc` must print 6.
9. Run `cmake --build build --config Release --parallel`. Must exit 0.
10. Run `pre-commit run --files custom/custom.qrc custom/res/Custom/qml/QGroundControl/AppSettings/HelpSettings.qml custom/res/Custom/qml/QGroundControl/FlyView/PreFlightSoundCheck.qml`. Must exit 0.
11. Run the audit-check `rg` and capture its count.
12. Return per the output contract.
</verification_loop>
