# Phase 1 — Custom Folder Bootstrap + Branding

> Picks up after `docs/prompts/phase-0.md` (the original kickoff). Phase 0 deliverables (architecture overview, ADRs, patch ledger) must be in place before starting. Read `docs/architecture/00-overview.md` §2 (override architecture) before touching any file.

**Phase 1 deliverable in one sentence:** A branded build boots end-to-end with Sprig identity on every visible surface, with **zero entries in `UPSTREAM_PATCHES.md`**. If a slice forces an `UPSTREAM_PATCHES.md` entry to land, stop and refactor toward `custom/` — that's the whole point of this phase.

---

## Pre-flight decisions (user must provide before specific slices unblock)

| Block | Slice that needs it | Notes |
|---|---|---|
| **Codename** | 1.2 onward | Kickoff defers ("Avicontrol v2 or pick something better"). Working: "Sprig GCS". Commit before Slice 1.2 so the app name lands once, not twice. Register in `README.md` per kickoff. |
| **Sprig palette hex values** | 1.3 | Kickoff says pull from sprigaerospace.com. Need: primary, accent (green), background dark, background light, surface, error, warning, success, text-on-dark, text-on-light, disabled. Map to `QGCPalette` color names (see slice 1.3 task list). |
| **Logo source files** | 1.5 | SVG ideal (scales for splash, app icon, about box, toolbar). Need: light variant (on dark UI), dark variant (on light UI), mark-only (compact toolbar). |
| **About-box copy** | 1.2 | One paragraph. Will appear under the logo in the About dialog. |
| **Bebas Neue + Barlow font files** | 1.4 | Need: Bebas Neue Regular + Bold; Barlow Regular + Medium + SemiBold + Bold. License-compatible files for redistribution (OFL or similar — both fonts are OFL-licensed; pull from Google Fonts or the foundry). |

If any of these aren't ready when a slice starts, the slice can land with placeholder content (e.g. solid colors, "Sprig GCS" text logo, default Qt fonts) and the placeholder gets a TODO comment. Phase 1 cannot exit until all placeholders are replaced.

---

## Phase 1 slicing

Seven slices, ordered for build-greenness. Each slice ends with a CMake configure + build pass and a screenshot capture. Each slice should be a single small PR / branch within `sprig/main`.

### Slice 1.1 — Bootstrap the `custom/` override pipeline

**Goal:** Prove the seam works. The branded build is reachable; `SprigCorePlugin::instance()` is the active core plugin at runtime. Zero visual change yet.

**Tasks:**
1. Copy `custom-example/` → `custom/`. Keep `custom-example/` clean and upstream-tracked.
2. In `custom/CMakeLists.txt`, change `CUSTOMHEADER` and `CUSTOMCLASS` defines from `CustomPlugin` → `SprigCorePlugin`. (See `custom-example/src/CustomPlugin.cc` lines that reference these — same pattern, renamed.)
3. Rename `custom/src/CustomPlugin.{h,cc}` → `custom/src/SprigCorePlugin.{h,cc}`. Rename the class `CustomPlugin` → `SprigCorePlugin`. Update `custom.qrc` if it references the file by name.
4. Rename `CustomFirmwarePlugin*` → `SprigFirmwarePlugin*` and `CustomAutoPilotPlugin` → `SprigAutoPilotPlugin` in `custom/src/FirmwarePlugin/` and `custom/src/AutoPilotPlugin/`. Update `CustomFirmwarePluginFactory.cc` global static.
5. In `SprigCorePlugin::init()`, add a `qCInfo(...) << "Sprig GCS core plugin active"` log line so we can verify runtime activation.
6. Update top-level `.gitignore` if needed — `custom/` should NOT be gitignored for our fork (kickoff §3.5 of overview: we populate `custom/` as the live fork-specific folder). Confirm no `custom/` entry exists.
7. CMake configure + build on macOS. The `CMakeLists.txt:48–53` auto-detect sets `QGC_CUSTOM_BUILD=ON`. Verify in CMake output: `"QGC: Custom build directory detected"`.

**Validation:**
- Build succeeds on at least one platform (macOS preferred for fastest feedback loop; Linux/Windows/Android validation lands in Slice 1.7).
- Launch app; the Sprig log line appears in console.
- App visually identical to stock QGC (no branding changes yet — that's correct).

**Exit:** Branch `sprig/phase1-bootstrap` merged. Screenshot of unchanged-looking app committed as `docs/screenshots/phase1/00-bootstrap-no-visual-change.png` to prove the seam doesn't break anything.

**Patch ledger budget:** 0.

---

### Slice 1.2 — Branding metadata (names, strings, version, about-box)

**Goal:** App name, organization, copyright, version channel, and about-box text are all Sprig identity. Nothing visual yet beyond strings.

**Prereqs:** Codename committed. About-box copy from user.

**Tasks:**
1. In `custom/cmake/CustomOverrides.cmake`:
   - `QGC_APP_NAME "Sprig GCS"` (or codename)
   - `QGC_ORG_NAME "Sprig Aerospace"`
   - `QGC_ORG_DOMAIN "sprigaerospace.com"`
   - `QGC_APP_COPYRIGHT "© 2026 Sprig Aerospace"`
   - Confirm these variables flow into the Qt application identity and installer metadata. (Grep `CMakeLists.txt` and `cmake/` for the canonical names; they may differ slightly — fix the names in `CustomOverrides.cmake` to match.)
2. In `SprigCorePlugin`, override `stableVersionCheckFileUrl()` and `stableDownloadLocation()` to point to Sprig's update channel (placeholder URL — actual auto-update infra is Phase 7).
3. Audit the About dialog QML (find via `grep -rn "About" src/UI/*.qml src/UI/*.cc`). The about-text source is typically in a Q_PROPERTY on a controller. Override the controller's about text in `SprigCorePlugin` or shadow the About QML file. **Prefer the override; falling back to QML shadowing if no string hook exists.**
4. Window title: typically `Qt.application.name` in the root QML. If we set `QGC_APP_NAME` correctly in CMake, this should already follow. Verify.
5. Settings → "General" section: there's typically an "About" / "Version" subsection. Confirm it reads from Qt application metadata; if it has hardcoded "QGroundControl" strings, those go on the audit list for Slice 1.6.

**Validation:**
- Window title reads "Sprig GCS".
- About dialog shows Sprig copy + "© 2026 Sprig Aerospace".
- macOS: Dock label, Cmd-Q quit menu, application menu name all read "Sprig GCS".
- Build settings dir on the OS user profile is now `Sprig Aerospace/Sprig GCS` (or whatever Qt expects — verify on macOS via `~/Library/Preferences/com.sprigaerospace.SprigGCS.plist` or similar).

**Exit:** Screenshots: window title, About dialog, Settings → General. Commit as `docs/screenshots/phase1/01-branding-metadata-*.png`.

**Patch ledger budget:** 0. If the About-box controller has no string hook, shadow its QML in `custom/res/Custom/` rather than patching `src/`.

---

### Slice 1.3 — Color palette

**Goal:** Every QGC surface uses the Sprig palette. Dark industrial base, Sprig green accent.

**Prereqs:** Hex values from sprigaerospace.com.

**Tasks:**
1. In `SprigCorePlugin::paletteOverride(const QString &colorName, QGCPalette::PaletteColorInfo_t &colorInfo)` (already exists from custom-example template — extend it):
   - Build a `QHash<QString, QGCPalette::PaletteColorInfo_t>` keyed by `colorName`. The map is consulted for both `Light` and `Dark` themes and both `Disabled` and `Enabled` color groups.
   - To enumerate the color names QGC uses, grep `src/QmlControls/QGCPalette.cc` for the `_setColor` calls — that's the canonical list. Expect ~30 names: `windowShade`, `windowShadeDark`, `text`, `warningText`, `button`, `buttonText`, `buttonHighlight`, `primaryButton`, `primaryButtonText`, `textField`, `textFieldText`, `mapButton`, `mapButtonHighlight`, `mapWidgetBorderLight/Dark`, `brandingPurple`, `brandingBlue`, `colorGreen`, `colorOrange`, `colorRed`, `colorYellow`, `colorWhite`, etc.
   - Sprig dark theme: deep grey/black base, Sprig green for primary actions, white text. Sprig light theme: optional in Phase 1 (kickoff says default dark, light "available via setting") — if cycles are tight, ship dark only and document the light gap.
2. **Brand color names:** override `brandingPurple` and `brandingBlue` (QGC's brand colors) to Sprig green — those names appear in QML referenced as `qgcPal.brandingPurple` etc. Don't rename them in QML (that needs a source patch); just remap the colors.
3. Set `appSettings.indoorPaletteName` default to "Dark" via `SprigCorePlugin::adjustSettingMetaData()` for the relevant Fact (find the setting key via `grep -rn "indoorPaletteName" src/Settings/`).

**Validation:**
- Boot screen, fly view, plan view, settings, analyze, vehicle setup — all show Sprig palette.
- No raw QGC purple/blue remaining anywhere in the UI.
- Buttons, sliders, toolbar all themed.

**Exit:** Screenshots of all five main views in dark Sprig palette: `docs/screenshots/phase1/02-palette-{boot,fly,plan,settings,setup}.png`.

**Patch ledger budget:** 0. `paletteOverride()` is a stock virtual — no source patches needed.

---

### Slice 1.4 — Typography (Bebas Neue + Barlow)

**Goal:** Sprig fonts loaded and applied as the default UI fonts.

**Prereqs:** Font files (Bebas Neue Regular + Bold; Barlow Regular/Medium/SemiBold/Bold).

**Tasks:**
1. Place font files under `custom/res/Custom/fonts/`. Add them to `custom.qrc` under prefix `/Custom/fonts/`.
2. In `SprigCorePlugin::init()` (runs once at startup), load each font via `QFontDatabase::addApplicationFontFromData(...)` reading the QRC file. Log success/failure per font.
3. Set the default app font: `QGuiApplication::setFont(QFont("Barlow", defaultPt))` in `SprigCorePlugin::init()` after loading.
4. **Display font (Bebas Neue) usage:** kickoff specifies Bebas Neue for "display" — i.e. headers, status strip values, large numerical readouts (airspeed, altitude). These live in instrument widgets (`src/FlyView/`, `src/UI/toolbar/*Indicator.qml`). Since QML widgets read fonts via `ScreenTools` or direct `font.family:` — Phase 2 will swap these comprehensively. **For Phase 1, only swap the base font** (Barlow as default). Bebas Neue is registered with QFontDatabase so it's available, but applying it widely is Phase 2 work. If you find a `Q_PROPERTY` on `ScreenToolsController` for the default font family, override the default to Barlow there.
5. Verify `ScreenTools.defaultFontPixelHeight/Width` calculations still produce sane values with Barlow (it has different metrics than Qt's default).

**Validation:**
- Spot-check buttons, labels, settings text — they render in Barlow.
- Bebas Neue listed in `QFontDatabase::families()` at startup (verify via debug log).
- DPI / sizing across all views still looks correct on macOS Retina + a 1080p external display.

**Exit:** Screenshots: settings page (lots of text — easy to verify font), fly view, plan view. `docs/screenshots/phase1/03-typography-*.png`.

**Patch ledger budget:** 0. If `ScreenToolsController` requires a patch to swap the default font, **stop** — there's almost certainly a settings-level path that doesn't require it.

---

### Slice 1.5 — Logos, splash, app icon, about-box graphic

**Goal:** Every brand-image surface is Sprig.

**Prereqs:** Logo SVGs (light + dark + mark).

**Tasks:**
1. **Splash screen.** The splash QML lives in `src/UI/` or is set as the initial window content. Find via `grep -rn "Splash\|splash" src/UI/*.qml`. Replace with a custom splash via `custom.qrc` + URL interceptor shadowing, or via a `SprigCorePlugin` hook if one exists for boot screen.
2. **Toolbar logo.** `FlyViewToolBar.qml` and `PlanViewToolBar.qml` have a logo region (see `src/UI/toolbar/` — first child in the row layout). Find the source image reference and shadow it via QRC.
3. **About dialog logo.** Same approach as 1.2's about text — locate the QML, override either the image source via `QGCOptions`-style hook or shadow the QML.
4. **App icons (per platform):**
   - macOS `.icns`: replace `custom/res/Icons/macos.icns` (custom-example pattern). The CMake branding var `QGC_MACOS_ICON_PATH` points to it.
   - Windows `.ico`: same pattern, `custom/res/Icons/windows.ico` → `QGC_WINDOWS_ICON_PATH`.
   - Linux: PNG sizes per freedesktop spec; check `deploy/linux/` for size list.
   - Android: replace `custom/android/res/mipmap-*/` PNGs (the CMake hook at `custom/CMakeLists.txt` for Android overlay handles this automatically).
   - iOS: deferred to Phase 7 per kickoff scope.
5. **Installer graphics (Windows NSIS, macOS DMG):** replace via custom CMake vars in `CustomOverrides.cmake`. The variable names follow the pattern `QGC_INSTALLER_*`. Pull from `cmake/QGCInstaller.cmake` (or wherever installer is configured) for the canonical names.
6. **About-box logo** — the image that appears next to the about text. Same shadow-or-hook pattern.

**Validation:**
- Boot the app: splash shows Sprig logo.
- Toolbar shows Sprig mark (top-left of fly + plan views).
- Settings → About: Sprig logo above copy.
- macOS dock icon: Sprig icon.
- macOS Finder Get-Info on the .app: Sprig icon.
- Android: install on a device, app drawer shows Sprig icon. (If no Android device handy, build the APK and inspect the AndroidManifest + res/mipmap output.)
- Windows: build the installer, run setup, verify icon in Add/Remove Programs + Start menu.

**Exit:** Screenshots: splash, fly-view toolbar close-up, plan-view toolbar close-up, about dialog. Platform installer screenshots (macOS DMG, Windows installer header) added to `docs/screenshots/phase1/04-icons-*.png`.

**Patch ledger budget:** 0.

---

### Slice 1.6 — Hidden-strings audit (the "zero QGC remaining" pass)

**Goal:** No "QGroundControl", "QGC", or upstream vendor preset strings remain on any user-visible surface.

**Tasks:**
1. **Build a string ledger.** `rg -i "qgroundcontrol|\\bqgc\\b" --type qml --type cpp -g '!src/{MAVLink,Comms,Vehicle,FirmwarePlugin,AutoPilotPlugins,FactSystem,Settings}/**' src/UI src/FlyView src/PlanView src/AnalyzeView src/QmlControls > /tmp/qgc-strings.txt`. Walk the output.
2. For each user-visible occurrence, choose the cheapest fix in this priority:
   1. **CorePlugin string hook** — if one exists, use it.
   2. **QML shadow** — drop the file into `custom/res/Custom/qml/QGroundControl/<Module>/<File>.qml` with the string changed; the URL interceptor picks it up.
   3. **Translation override** — Qt's `tr()` strings can be overridden via a custom `.qm` file loaded in `SprigCorePlugin::init()`. This is the lightest touch when the only thing changing is a single user-facing string in a file that's otherwise upstream-clean. **Preferred for one-off strings** — shadowing a whole QML file just to change one label is wasteful.
   4. **Source patch** — last resort. Adds a patch ledger entry. Avoid.
3. **Vendor video preset hide.** `VideoSettings.h:53–64` and `VideoManager.cc:505–709` enumerate hardcoded vendor source names (3DR Solo, Parrot Discovery, Yuneec Typhoon, Herelink). These appear in the Video → Source dropdown. Either:
   - Override via `SprigCorePlugin::adjustSettingMetaData()` for the `videoSource` Fact to filter the enum values, OR
   - Shadow the Video Settings QML page to hide the dropdown entries Sprig doesn't ship.
   Document which approach is used; the former is cleaner.
4. **Plan creators list** — `QGCCorePlugin::planCreators()` returns the "what kind of plan are you making" cards on the Plan view. Stock QGC includes BlankPlanCreator, SurveyPlanCreator, CorridorScanPlanCreator, StructureScanPlanCreator. For Phase 1, keep all four but make sure none have visible "QGC" branding. (Phase 3 adds Sprig ag plan creators.)
5. **First-run prompt** — `QGCCorePlugin::firstRunPromptStdIds()` shows initial setup dialogs. Audit the prompt content; override via `firstRunPromptResource(int id)` if any QGC strings appear.

**Validation:**
- Re-run the `rg` audit. The only remaining matches should be in code/comments (developer-facing), not in user-visible strings.
- Walk every menu, every dialog, every error message Sprig is likely to see. Note any survivors and fix them.
- Spot-check on Android (touch UI surfaces different elements).

**Exit:** Updated `rg` audit log committed at `docs/screenshots/phase1/05-strings-audit-clean.txt` showing no user-visible hits. Screenshots of any non-obvious renamed surfaces.

**Patch ledger budget:** 0. If a string can only be reached by a source patch, write it down in a Slice 1.6-blockers note and bring it back to the user before adding to ledger.

---

### Slice 1.7 — Three-platform validation + regression baseline

**Goal:** Sprig build is green on Linux x86_64, Windows, and Android arm64. Full screenshot regression baseline captured.

**Prereqs:** Qt 6 SDKs installed for each target. Android NDK + SDK for the mobile build.

**Tasks:**
1. **Linux x86_64.** Configure + build per `AGENTS.md` "Build & Test" section: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release --parallel`. Run unit tests: `cd build && ctest --output-on-failure -L Unit --parallel $(nproc)`. Capture screenshots on Linux (in a VM or container if needed).
2. **Windows.** Same on a Windows VM / runner. Build the installer (`cmake --build build --target package` or the CMake CPack config).
3. **Android arm64.** Use the standard QGC Android build steps. Install on a physical device or emulator. Verify launch, fly-view chrome, plan-view chrome, settings.
4. **Pre-commit.** Run `pre-commit run --all-files` per AGENTS.md. Fix any lint issues introduced by Phase 1 work (clang-format, qmllint, etc.).
5. **Side-by-side screenshots.** For each major view (boot, fly, plan, settings, vehicle setup, analyze) take screenshots of both stock QGC (a clean build of upstream `main`) and Sprig GCS. Commit pairs as `docs/screenshots/phase1/regression/<view>-{stock,sprig}.png`.
6. **Patch ledger validation.** `cat UPSTREAM_PATCHES.md` — must show zero active patches. If any landed during Phase 1, Phase 1 exit is blocked until they're refactored into `custom/`.

**Exit:** All three platforms build green. Regression baseline committed. Tag the merge as `phase1-complete` so we can diff Phase 2 against it.

**Patch ledger budget:** 0. (See — that's the whole point.)

---

## Phase 1 exit criteria (must all be true)

- [ ] Build green on Linux, Windows, Android (macOS optional in this phase but typically validated as the dev-loop platform).
- [ ] `pre-commit run --all-files` clean.
- [ ] Unit tests pass.
- [ ] `UPSTREAM_PATCHES.md` has zero active entries.
- [ ] Side-by-side screenshots of all major views committed under `docs/screenshots/phase1/regression/`.
- [ ] No "QGroundControl" or "QGC" strings visible to the user on any surface walked in Slice 1.6.
- [ ] App name, window title, dock label, About dialog, splash, app icon, toolbar logo all read/show Sprig identity.
- [ ] Sprig palette applied across all main views.
- [ ] Barlow loaded as default UI font; Bebas Neue available for Phase 2 display use.
- [ ] Sprig green accent visible (replaces QGC purple) on primary buttons, selection states, status indicators.
- [ ] Tag `phase1-complete` placed on the merge commit.

---

## Handoff to Phase 2

Phase 2 begins from `phase1-complete`. The work is restyling the major UI surfaces (toolbar, instrument cluster, fly view chrome, plan view chrome, base control library) to match the Flutter GCS aesthetic.

**Phase 2 will likely break the "zero patches" rule** — `FlyView.qml:174–178` and `PlanView.qml:228–232` hardcode toolbar children with no `Loader` seam. The first Phase 2 decision is whether to (a) shadow the entire view QML files in `custom/` (no patch, but Sprig owns the file forever and must re-merge upstream changes manually), or (b) add `Loader`-based chrome in `src/` (patch ledger entries PATCH-002/-003, but the change is upstreamable as a generic improvement).

That decision belongs in Phase 2's kickoff prompt at `docs/prompts/phase-2.md` and should reference the screenshots captured here as the visual delta target.

---

## First Action

Open a new branch:

```
git checkout -b sprig/phase1-bootstrap
```

Then start Slice 1.1. Each slice is a self-contained commit (or small PR) within `sprig/phase1-bootstrap`; merge the whole branch back to `sprig/main` once Slice 1.7 exits clean. Don't try to one-shot all seven slices in a single session — work one, screenshot it, commit it, move on.
