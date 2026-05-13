# qgroundcontrol Review Rubric

Use this rubric in Phase 5 of the orchestrator workflow. Reviewers (Claude,
Codex, MiMo, Gemini) read this alongside the diff and `CLAUDE.md`, `AGENTS.md`,
`CODING_STYLE.md`, `test/TESTING.md`, and `.pre-commit-config.yaml`. They
produce a structured issue list. **Findings under "high-weight zones" are
elevated in synthesis even when only one reviewer flags them.**

Severity tags reviewers should use:
- **BLOCKER** — must fix before merge; would cause incorrect flight behavior,
  vehicle-state corruption, null dereference of `activeVehicle()`, break
  Windows / macOS / Android / iOS users, violate the Fact System or
  FirmwarePlugin contract, leak credentials, or regress pinned behavior
- **MAJOR** — should fix before merge; real bug, semantic gap, missing test
  for new behavior, public-header drift in `src/FactSystem/Fact.h` /
  `src/Vehicle/Vehicle.h` / `src/FirmwarePlugin/FirmwarePlugin.h` not
  captured by downstream subclasses
- **MINOR** — nice to fix; no functional impact
- **NIT** — style, naming, organization

Suppress NITs in synthesis unless multiple reviewers agree on the same one.

---

## High-weight zones (single-reviewer findings elevated)

These are paths where a single sharp reviewer catching an issue is enough to
act. Two LLMs trained on similar data will hallucinate the same wrong answer
here, so single hits matter.

1. **Fact System public contract** — `src/FactSystem/Fact.h`,
   `src/FactSystem/ParameterManager.*`. Fact metadata, signal/slot wiring,
   `setCookedValue` / `setRawValue` semantics. Every vehicle parameter
   consumer depends on this.
2. **Vehicle public surface** — `src/Vehicle/Vehicle.h` and every consumer
   of `MultiVehicleManager::instance()->activeVehicle()`. Null-check is
   enforced by the `vehicle-null-check` pre-commit hook; missing checks are
   BLOCKER even on a one-line patch.
3. **FirmwarePlugin abstraction** — `src/FirmwarePlugin/FirmwarePlugin.h` and
   the PX4 / ArduPilot subclasses. Any firmware-specific behavior must route
   through `vehicle->firmwarePlugin()`. Inline branching on firmware type
   outside this layer is BLOCKER.
4. **MAVLink routing** — `src/MAVLink/`. `target_system` / `target_component`
   selection, message id assignment, multi-vehicle dispatch, sequence-number
   continuity on mission upload.
5. **MissionManager** — `src/MissionManager/`. Mission upload state machine,
   geofence/rally upload, mission-item type discrimination, retry on
   `MISSION_REQUEST_INT` timeout, `MISSION_ACK` validated for accepted
   status (not merely non-error).
6. **QML sizing & color contract** — `ScreenTools.defaultFontPixelHeight/Width`
   for sizing; `QGCPalette` for colors. Hardcoded pixel values or color
   literals are BLOCKER. See AGENTS.md golden rules and CODING_STYLE.md.
7. **`Q_ASSERT` ban** — enforced by the `check-no-qassert` pre-commit hook.
   Replace with logged error + graceful return.
8. **Cross-platform CI** —
   `.github/workflows/{linux,macos,windows,android,ios}.yml`, plus composite
   actions under `.github/actions/`. Drift between platform workflows is a
   common silent-break source.
9. **Pre-commit gate** — `.pre-commit-config.yaml`: clang-format, clang-tidy,
   ruff, pyright, shellcheck, actionlint, zizmor, qmllint, clazy,
   vehicle-null-check, check-no-qassert. Any change must still pass
   `pre-commit run --all-files`.
10. **Python tooling layer** — `tools/` (uv-managed) and `.github/scripts/`
    (httpx + jinja2 for API access and templating). Bootstrap scripts
    (`install_dependencies.py`, `ccache_helper.py`) must be **stdlib-only** —
    they run before deps are installed. See AGENTS.md "CI Conventions".

---

## 1. Fact System (`src/FactSystem/`)

### Contract stability
- [ ] Does the diff change a public method or `Q_PROPERTY` on `Fact` /
      `ParameterManager` / `FactMetaData`? If yes, are ALL consumers in
      `src/Vehicle/`, `src/AutoPilotPlugins/`, `src/MissionManager/`,
      `src/QmlControls/`, and `src/Settings/` updated in the same PR?
- [ ] New optional methods are fine. Renamed or removed methods, or changed
      signal signatures, are BLOCKER unless all consumers are updated.

### Parameter storage discipline
- [ ] **No custom parameter storage.** All vehicle parameters route through
      `vehicle->parameterManager()->getParameter(...)`. Ad-hoc `QVariant`
      caches or static maps in unrelated subsystems are BLOCKER.
- [ ] Metadata (units, range, decimals, default) defined once, not re-derived
      at call sites?

### Signals & slots
- [ ] `Fact::valueChanged` listeners disconnected on object destruction or
      via parent ownership? No dangling lambdas capturing raw pointers?
- [ ] `Q_PROPERTY` NOTIFY signals declared and emitted on every write path?

---

## 2. Vehicle & MultiVehicle (`src/Vehicle/`)

### MultiVehicle null-check (BLOCKER if missing)
- [ ] **Every call site** of `MultiVehicleManager::instance()->activeVehicle()`
      null-checks before dereferencing. The `vehicle-null-check` pre-commit
      hook catches the obvious cases; reviewer eyes catch the subtle ones
      (e.g., null-check on line 10, four-line gap, deref on line 14 after a
      function call that could have detached the vehicle).
- [ ] QML access uses
      `property var vehicle: QGroundControl.multiVehicleManager.activeVehicle`
      followed by an `enabled: vehicle && ...` guard?

### Vehicle lifecycle
- [ ] Vehicle destruction ordering: FirmwarePlugin teardown happens before
      MAVLink link teardown, not after?
- [ ] Signal disconnections on vehicle disconnect — no slot fires on a
      partially-destroyed Vehicle?
- [ ] No new global mutable state outside `Vehicle` / `MultiVehicleManager`
      that holds vehicle-derived data (such state goes stale on disconnect)?

### Comms
- [ ] Telemetry rate changes route through the existing rate-control path?
- [ ] No hardcoded sysid/compid — pulled from `Vehicle` or `MAVLinkProtocol`?

---

## 3. FirmwarePlugin (`src/FirmwarePlugin/`)

### Abstraction discipline (BLOCKER if violated)
- [ ] **No `vehicle->firmwareType() == MAV_AUTOPILOT_PX4` (or similar) outside
      `src/FirmwarePlugin/`.** Firmware-specific behavior MUST be a virtual
      method on `FirmwarePlugin` with PX4 / ArduPilot overrides. Inline
      firmware branching in `src/Vehicle/`, `src/MissionManager/`, or
      `src/QmlControls/` is BLOCKER.
- [ ] New behavior added to one subclass also covered in the other (or the
      base class returns a sensible default and the diff explains why one
      firmware needs the override)?

### Public header changes
- [ ] If `FirmwarePlugin.h` virtual signature changed, both `PX4FirmwarePlugin`
      and `APMFirmwarePlugin` (and any custom build subclasses) updated in
      the same PR?
- [ ] New pure-virtuals avoided unless every existing subclass implements
      them in the same diff?

---

## 4. MAVLink (`src/MAVLink/`)

### Message construction
- [ ] `target_system` is the **vehicle** sysid (not the GCS sysid)?
- [ ] `target_component` is the correct component id
      (`MAV_COMP_ID_AUTOPILOT1` for most flight-controller messages)?
- [ ] Sysid/compid consistent across multi-message sequences (mission upload,
      parameter download)?

### Routing
- [ ] Multi-vehicle: messages dispatched to the right Vehicle by sysid, not
      broadcast?
- [ ] Heartbeat handling preserves the existing vehicle-discovery flow?

### Tests
- [ ] New message construction tested with golden bytes where feasible
      (byte-for-byte verified encoding)?
- [ ] Timeouts configurable, not magic numbers?

---

## 5. MissionManager (`src/MissionManager/`)

### Upload state machine
- [ ] Sequence numbers monotonic, starting at 0?
- [ ] Bounded retries on `MISSION_REQUEST_INT` timeout?
- [ ] `MISSION_ACK` checked for `MAV_MISSION_ACCEPTED`, not just non-error?
- [ ] Geofence / rally uploads kept distinct from the standard mission state
      machine — no cross-contamination?

### Mission items
- [ ] Item-type discrimination handles all `MAV_CMD_*` variants the UI can
      emit, with a fallback for unknown commands?
- [ ] Coordinate frames (`MAV_FRAME_*`) preserved on round-trip
      upload/download?

---

## 6. AutoPilotPlugins (`src/AutoPilotPlugins/`)

- [ ] New setup pages register through the existing AutoPilotPlugin facade?
- [ ] Vehicle parameter access goes through Facts; no `QSettings` for
      per-vehicle state?
- [ ] Firmware-specific setup pages live under the right subclass
      (`PX4/` vs `APM/`), not in shared code?

---

## 7. QmlControls (`src/QmlControls/`) and QML at large

### Sizing & colors (BLOCKER if violated)
- [ ] **No hardcoded pixel values.** Use
      `ScreenTools.defaultFontPixelHeight` / `defaultFontPixelWidth` and
      their multiples.
- [ ] **No hardcoded color literals (`"#RRGGBB"` or `Qt.rgba(...)`)**. Use
      `QGCPalette` (e.g., `qgcPal.window`, `qgcPal.text`, `qgcPal.button`).

### Bindings & ownership
- [ ] `Q_PROPERTY` NOTIFY signals wired so QML bindings update correctly?
- [ ] No JS-side caching of Fact values that could go stale (bind directly
      to the Fact)?

### qmllint
- [ ] `pre-commit run qmllint --files <paths>` clean?
- [ ] No new QML files outside the expected `src/.../qml/` locations?

---

## 8. Settings (`src/Settings/`)

- [ ] New settings registered through the existing SettingsManager facade?
- [ ] No bare `QSettings::setValue` in unrelated subsystems?
- [ ] Defaults defined alongside the setting declaration, not duplicated at
      read sites?

---

## 9. C++20 / coding style

### Per CODING_STYLE.md
- [ ] Naming follows the documented conventions (PascalCase types,
      camelCase methods, `m_` member prefix where the existing file uses
      it — match surrounding style)?
- [ ] C++20 features used appropriately (no regression to pre-C++20
      idioms when the surrounding code uses concepts / ranges / `consteval`)?
- [ ] `qCDebug` / `qCWarning` for logging, not bare `qDebug()` at call
      sites in shipping code?

### `Q_ASSERT` ban (BLOCKER if violated)
- [ ] **No new `Q_ASSERT` / `Q_ASSERT_X`.** Replace with logged error +
      graceful return. The `check-no-qassert` pre-commit hook will reject
      otherwise, but reviewer eyes catch it earlier.

---

## 10. Tests (`test/`, per `test/TESTING.md`)

### Discipline
- [ ] New tests derive from the project's QTest base classes (see
      `test/TESTING.md`) rather than raw `QObject`?
- [ ] CTest label registered correctly (`Unit` vs `Integration`)?
- [ ] Asynchronous flows use `MultiSignalSpy` from `test/TESTING.md`, not
      bare `QSignalSpy` plus ad-hoc waits?
- [ ] Every new public method / signal / Fact has at least one test?

### Forbidden patterns
- [ ] No `QSKIP` on platforms the test is meant to cover?
- [ ] No `QTest::qSleep(...)` where `MultiSignalSpy::wait(...)` would work?
- [ ] No tests that depend on real network, real serial ports, or a real
      vehicle?

---

## 11. CMake & build (`CMakeLists.txt`, `cmake/`)

- [ ] Build still configures cleanly with
      `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`?
- [ ] New source files added to the right module's `target_sources(...)`?
- [ ] No new third-party dependency added without an open question /
      architect sign-off?
- [ ] ccache helper invocation preserved on platform workflows?

---

## 12. Python tooling (`tools/`, `.github/scripts/`)

### CI scripts
- [ ] HTTPX used for GitHub API access (with `gh` CLI fallback via
      `common.gh_actions`)?
- [ ] Jinja2 templates live under `.github/scripts/templates/`?
- [ ] Version numbers / build settings read from `.github/build-config.json`
      via `common.build_config.get_build_config_value()` — not hardcoded?
- [ ] `$GITHUB_OUTPUT` writes go through `common.gh_actions.write_github_output()`?

### Bootstrap scripts
- [ ] `install_dependencies.py` and `ccache_helper.py` remain **stdlib-only**
      (they run before deps are installed)?

### `tools/` Python
- [ ] uv extras (`scripts`, `test`) used per `tools/README.md`?
- [ ] Tests run via `uv run --extra scripts --extra test pytest tests/ -q`?

---

## 13. CI / GitHub Actions (`.github/workflows/`, `.github/actions/`)

- [ ] Platform workflows (`linux.yml`, `macos.yml`, `windows.yml`,
      `android.yml`, `ios.yml`) consistent — changes touch all relevant
      platforms or explain the omission?
- [ ] Reusable workflows and composite actions used over ad-hoc inline
      steps where one exists (cmake-configure, cmake-build, run-unit-tests,
      qt-install, etc.)?
- [ ] zizmor and actionlint clean on new workflow changes?

---

## 14. Cross-cutting

### Security / safety
- [ ] No hardcoded credentials, API keys, or PII?
- [ ] No `.env` or secret-bearing files committed?
- [ ] User-facing safety-critical commands (arm / disarm / mode change /
      mission start) preserved as explicit user actions, not auto-triggered?

### Documentation
- [ ] AGENTS.md / CODING_STYLE.md / test/TESTING.md updated if conventions
      changed?
- [ ] Public C++ API documented (Doxygen comments) where the surrounding
      code is documented?

### Conventions
- [ ] Conventional commit prefix (`feat:`, `fix:`, `refactor:`, `docs:`,
      `test:`, `chore:`, `perf:`, `ci:`)?
- [ ] Commit scope matches the affected subsystem (`vehicle`, `factsystem`,
      `firmwareplugin-px4`, `mission`, `mavlink`, `qmlcontrols`, `tools`,
      `ci`)?

---

## Reviewer self-check before submitting findings

- Did I read `AGENTS.md` for the golden rules (Fact System, MultiVehicle
  null-check, FirmwarePlugin abstraction, QML sizing/colors) before
  flagging style issues?
- Did I read `CODING_STYLE.md` and `test/TESTING.md` before flagging
  conventions or test-shape issues?
- Did I check `.pre-commit-config.yaml` to know which hooks gate the
  commit (so I don't flag something the linter will catch automatically)?
- Are my BLOCKER findings actually blockers, or am I being conservative?
- Did I distinguish between "this is wrong" and "I'd write it differently"?
- Did I check whether the plan declared something out of scope before
  flagging it?
