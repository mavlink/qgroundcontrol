# Plan: Phase 1 / Slice 1.6 — Hidden-strings audit (zero QGC remaining)

Source issues: Strattoon/qgroundcontrol#11 (umbrella), #6 (this slice's deliverable).

## Goal

Every user-visible "QGroundControl", "QGC", and vendor-preset string is replaced
with Sprig identity or hidden from the UI. All fixes live exclusively in
`custom/`. Zero `UPSTREAM_PATCHES.md` entries. The audit log is committed.

## Behavior contract

1. Android app launcher label reads "Sprig GCS", not "Custom QGroundControl".
2. Android package ID is `com.sprigaerospace.spriggcs` (was `org.mavlink.qgroundcontrol`).
3. Settings → Video → Source dropdown does not show "3DR Solo", "Parrot Discovery", "Yuneec Mantis G", "Herelink AirUnit", or "Herelink Hotspot".
4. `rg -i "qgroundcontrol|\bqgc\b" --type qml --type cpp custom/` returns zero user-visible (qsTr / android:label / plist) hits.
5. `docs/screenshots/phase1/05-strings-audit-clean.txt` exists with the rg output.
6. `UPSTREAM_PATCHES.md` has zero active entries (unchanged).

## Shared contracts

- `SprigCorePlugin::adjustSettingMetaData` (`custom/src/SprigCorePlugin.cc`) — sole site for all Fact-level overrides; video source filter is added here.
- `custom/android/AndroidManifest.xml` — sole home for Android app identity.

## Slices

### Slice A — Android manifest Sprig branding (Doc/config — orchestrator inline)

Files: `custom/android/AndroidManifest.xml` (MODIFY)

Acceptance:
- `grep -c 'label="Sprig GCS"' custom/android/AndroidManifest.xml` → 2
- `grep -c 'package="com.sprigaerospace.spriggcs"' custom/android/AndroidManifest.xml` → 1
- `grep -c 'QGroundControl\|org.mavlink.qgroundcontrol' custom/android/AndroidManifest.xml` → 0

In-scope: update `android:name` attributes containing fully-qualified old package path (e.g. `org.mavlink.qgroundcontrol.QGCActivity` → `com.sprigaerospace.spriggcs.QGCActivity`). Short-form `.QGCActivity` stays.

Out-of-scope: upstream `android/AndroidManifest.xml`; Java/Kotlin source.

Verification: `pre-commit run --files custom/android/AndroidManifest.xml`.

### Slice B — Video source vendor-preset filter (Refactor — jcode dispatch)

Files: `custom/src/SprigCorePlugin.cc` (MODIFY)

Extend `adjustSettingMetaData` to filter `VideoSettings::videoSource` enum to allowed sources only (NoVideo, Disabled, RTSP, UDP264, UDP265, TCP, MPEGTS). Verify exact constant names in `src/Settings/VideoSettings.h` and `FactMetaData::setEnumStrings`/`setEnumValues` signatures before writing.

Acceptance: build clean, pre-commit clean.

Verification: `cmake --build build --config Release --parallel && pre-commit run --files custom/src/SprigCorePlugin.cc`.

### Slice C — Full rg audit + clean log (Doc/config + optional QML shadow micro-fixes)

Files: `docs/screenshots/phase1/05-strings-audit-clean.txt` (CREATE); any QML shadows under `custom/res/Custom/qml/` if survivors found (CREATE); `custom/custom.qrc` (MODIFY — only if shadows added, re-verify all `<qresource>` blocks).

Procedure:
1. Run `rg -i "qgroundcontrol|\bqgc\b" --type qml --type cpp` over src/UI src/FlyView src/PlanView src/AnalyzeView src/QmlControls custom/
2. Filter to user-visible lines (qsTr / android:label / plist keys).
3. For each survivor: if in custom/, fix in-place; if in src/, fix via QML shadow in `custom/res/Custom/qml/` (max 5 new shadows without check-in); if shadow infeasible, surface for UPSTREAM_PATCHES.md decision.
4. Also rg `"3DR|Parrot|Yuneec|Herelink"` to catch JSON/XML vendor strings.
5. Commit final log to `docs/screenshots/phase1/05-strings-audit-clean.txt`.

Acceptance: log file exists, pre-commit + build clean, audit produces zero user-visible hits after fixes.

### Slice D — Close issue #6 (orchestrator inline)

Post comment on Strattoon/qgroundcontrol#6 with merge SHA + audit summary, close issue. Issue #11 stays open (Slice 1.7 still pending).

## Out of scope

- Slice 1.7 (three-platform validation, user-blocked on Qt SDKs).
- Issue #11 close.
- iOS branding (Phase 7).
- Light theme completion (Phase 2).
- Changing `QGC_APP_NAME = "Avicontrol"` — set intentionally in closed slice 1.2.

## Risks

- `FactMetaData` API mismatch — executor checks `src/FactSystem/FactMetaData.h` first.
- Android package rename impacts Play Store / sideloaded installs — internal dev builds only for Phase 1, safe.
- `custom.qrc` sibling block drop on edit — re-verify all `<qresource>` blocks after any write.

## Verification command (whole plan)

```bash
cmake --build build --config Release --parallel
pre-commit run --all-files
test -f docs/screenshots/phase1/05-strings-audit-clean.txt
grep -c 'label="Sprig GCS"' custom/android/AndroidManifest.xml  # → 2
```
