# Sprig GCS — Project Kickoff Prompt

> The master plan. Paste into Claude Code at the root of a fresh clone. Sets architectural discipline, lays out the phased build. Each phase has its own expanded prompt at `docs/prompts/phase-N.md` — work one phase at a time, don't try to eat the whole elephant.

---

## Identity

**Codename:** Avicontrol v2 (or pick something better — register it in `README.md` and stop calling it "the fork")
**Upstream:** `mavlink/qgroundcontrol` (Qt6 / QML, current main)
**Vendor:** Sprig Aerospace
**Primary target:** S1000 dodecacopter hybrid agricultural UAS (Cube Orange+ / PX4, CM5 mission computer, Rotax 912 ULS + AXM3 generator HV bus, MaxxECU / DTI HV-850 / ENNOID-BMS / Volz DA-26 on native CAN)
**Secondary targets:** future Sprig airframes, generic PX4 vehicles (don't break stock QGC capability)

---

## Non-Negotiable Principles

1. **`custom/` first.** Every customization that *can* live in `custom/` *must*. Touching anything outside `custom/` requires a `UPSTREAM_PATCHES.md` entry with rationale, upstream PR status, and rebase risk.
2. **Track upstream main.** Weekly rebase cadence. Patch ledger validated and re-applied on every rebase. Drift is the enemy.
3. **AI-native repo.** `CLAUDE.md` at root, `AGENTS.md` for multi-agent workflows, Understand Anything graph committed under `docs/knowledge-graph/`, architecture tours kept current.
4. **Upstream what's generic.** Anything not Sprig-specific (perf fixes, bug fixes, new generic mission item types) gets a PR back to upstream so we don't carry it forever.
5. **No vendored source.** Submodules or package manager for external deps. Period.
6. **Stock QGC must still work.** Don't break the baseline. Custom build is opt-in via `QGC_USE_CUSTOM_BUILD`.

---

## Phase 0 — Environment + Knowledge Graph (Day 1–2)

**Goal:** Repo cloned, builds green on three platforms, knowledge graph generated and committed, architecture documented well enough that any new task starts with reading instead of grep.

Tasks:
- Fork `mavlink/qgroundcontrol` → `sprig-aerospace/sprig-gcs`, branch `sprig/main` tracking upstream `main`
- Verify clean Qt6 CMake build on **Linux x86_64**, **Windows**, **Android arm64**
- Install Understand Anything plugin in Claude Code
- Run `/understand-anything` over the full tree, commit graph artifacts to `docs/knowledge-graph/baseline/`
- Generate architecture tours for: `MissionController` + `PlanMasterController` + complex mission items, `MAVLinkProtocol` + `Vehicle` + `LinkManager`, `FirmwarePlugin` + `AutoPilotPlugin` registration, custom build system (`custom-example/` + `QGC_USE_CUSTOM_BUILD` + `Custom/` resource prefix), QML resource layering, video pipeline (GStreamer + Qt RHI)
- Write `docs/architecture/00-overview.md` summarizing the tours in your own words — if you can't summarize it, you don't understand it
- Set up `UPSTREAM_PATCHES.md` (empty ledger), `CLAUDE.md` (working conventions), `AGENTS.md` (multi-agent rules), `docs/decisions/` (ADR directory)

**Deliverable:** Graph committed. Tours written. Three-platform build green. ADR-001 "Why we forked QGC instead of building Flutter GCS from scratch" written.

**Phase 0 retrospective (2026-05-12):** The architecture overview + ADRs landed; the knowledge graph was deferred (ADR-002) — generating a full graph on ~1,945 source files exceeds the skill's design envelope, so we shipped six focused subagent surveys synthesized into `docs/architecture/00-overview.md` instead. Three-platform build-green is still outstanding and rolls into the Phase 1 exit criteria (Slice 1.7).

---

## Phase 1 — Custom Folder Bootstrap + Branding (Week 1)

**Goal:** Branded build boots end-to-end with Sprig identity. No functional changes — pure branding pass to validate the override pipeline works.

Tasks:
- Create `custom/` following `custom-example/` pattern
- Subclass `CorePlugin` → `SprigCorePlugin`, register in `custom/src/CustomPlugin.cc`
- Custom QML resource dir at `custom/res/Custom/`
- Override `QGCPalette` with Sprig palette (dark industrial base, green accent — pull exact hex values from sprigaerospace.com)
- Typography: Bebas Neue (display) + Barlow (body) — bundle as resources, register via QFontDatabase
- Brand assets: logo (light + dark), splash, app icon (all platforms), about-box copy
- Boot screen → fly view → plan view → settings: every visible surface shows Sprig identity, zero QGC strings or icons remaining
- Screenshot every view to `docs/screenshots/phase1/` for regression baseline

**Deliverable:** Branded build, side-by-side screenshots vs stock QGC committed, zero entries in `UPSTREAM_PATCHES.md` (this phase should be 100% custom-folder-clean — if it isn't, stop and refactor).

**Expanded prompt with slices:** `docs/prompts/phase-1.md`.

---

## Phase 2 — UI Restyle to Match Flutter GCS (Weeks 2–4)

**Goal:** Operator-facing UI matches the Flutter GCS aesthetic. Internal panels (settings, parameter editor, log analyzer) stay close to stock QGC for now — restyle them in a later pass.

Tasks:
- Custom main toolbar (replaces `MainToolBar.qml`) — Sprig layout, Sprig controls
- Custom instrument widget cluster for fly view — airspeed, altitude, heading, battery, generator output, fuel — match Flutter GCS visual language
- Custom fly view chrome — overlays, status strip, mode indicator
- Custom plan view chrome — toolbar, mission list, item editor styling
- Custom QML control library at `custom/res/Custom/Controls/` — buttons, sliders, gauges with Sprig styling — replace `QGCButton`, `QGCSlider`, etc. via QML import path priority
- Dark/light mode behavior — default dark, light available via setting
- Compare every restyled view side-by-side with the Flutter GCS, document deltas in `docs/ui-parity-matrix.md`

**Deliverable:** UI parity matrix at 80%+ green. Patch ledger entries justified for every non-custom-folder change.

**Phase 0 note for Phase 2:** QML overrides in QGC use a URL interceptor, not import-path priority. Whole-file shadowing only. The kickoff text above is wrong on that detail — see `docs/architecture/00-overview.md` §2.2.

---

## Phase 3 — Custom Turn Patterns (Weeks 5–8)

**Goal:** Ag-specific transect turn patterns selectable from Plan view, generating valid PX4 missions.

Patterns to implement:
- **Racetrack turn** — 180° at end of swath, configurable radius
- **Teardrop turn** — bank-limited reversal, configurable approach geometry
- **Trombone turn** — extended approach for spray pattern continuity
- **Headland management** — auto-generated headland boundary, spray on/off integration
- **Skip-swath option** — racetrack with N-swath skip for spray drift management

Tasks:
- Subclass `TransectStyleComplexItem` for each pattern in `src/MissionManager/` (this is the upstream patch that *cannot* live in `custom/` — register it cleanly, document it in `UPSTREAM_PATCHES.md`, propose an upstream extension point PR so future patterns are pluggable)
- Add corresponding QML editor panels in `custom/res/Custom/MissionEditors/`
- Plan view visualization — turn geometry rendered correctly, mid-mission preview
- MAVLink mission item emission — validate against PX4 SITL and the S1000 sprig-sim-stack
- Mission validation — warn on unrealistic turn radii for given vehicle speed
- Unit tests for turn geometry math, integration tests for mission upload/download round-trip

**Deliverable:** All five patterns flying in PX4 SITL on stock Iris and on the S1000 dodecacopter airframe (`SYS_AUTOSTART=24001`). Upstream extension-point PR drafted (even if not merged).

---

## Phase 4 — Sprig Telemetry Overlays (Weeks 9–12)

**Goal:** S1000-specific telemetry visible in fly view, sourced from the CM5 mission computer via MAVLink.

Telemetry surfaces:
- **MaxxECU / Rotax** — RPM, MAP, EGT (per cylinder), CHT, fuel flow, ignition status, fault codes
- **AXM3 generator + HV bus** — generator RPM, output current, HV bus voltage, EVC500 contactor state, precharge status
- **ENNOID-BMS** — pack voltage, per-cell deltas, SoC, SoH, cell temps, balancing state, fault flags
- **DTI HV-850** — motor temps, inverter temps, per-motor current, fault flags
- **Volz DA-26** — throttle command vs feedback, lane health, heartbeat status
- **Flow controllers** — Satloc + SimpleLink — flow rate, target rate, boom state, tank level

Tasks:
- Define Sprig MAVLink dialect for compact telemetry packing (or use generic message channels — pick one, document the decision in an ADR)
- CM5 publishes telemetry via MAVLink router on Telem2 → Cube → ground
- Custom telemetry widgets in `custom/res/Custom/Telemetry/` — per-subsystem panels, configurable layout
- Fault overlay — any subsystem fault surfaces as a banner with acknowledge/dismiss
- Telemetry log integration — recorded to QGC's tlog for post-flight analysis

**Deliverable:** All six subsystems visible in real time during a HITL flight. Fault injection tests pass. Post-flight tlog parses cleanly.

**Phase 0 note for Phase 4:** Telemetry can be done with **zero `Vehicle.cc` patches** — register custom `FactGroup` subclasses via `SprigPX4FirmwarePlugin::factGroups()`. `NAMED_VALUE_FLOAT` is not auto-surfaced as Facts in stock QGC, so commit to the proper Sprig MAVLink dialect (pinned via CMake against a `sprig-aerospace/mavlink` fork) rather than the named-value shortcut. See `docs/architecture/00-overview.md` §2.4–2.5.

---

## Phase 5 — Ag-Specific Mission Features (Weeks 13–16)

**Goal:** GCS speaks ag, not just generic UAS.

Tasks:
- **Variable-rate prescription map import** — accept shapefile / GeoJSON / standard ag formats, render in Plan view as overlay, attach rate targets to swaths
- **Field boundary import** — same formats, generate exclusion zones automatically
- **Spray pattern preview** — visualize coverage on the field at planned altitude + boom width + flow rate
- **Tank planning** — estimate tank loads required, plan refill points, integrate with mission preview
- **Coverage report generation** — post-flight, export PDF/CSV with actual vs commanded rate, coverage map, anomalies

**Deliverable:** Plan a real S1000 mission against a real West Kentucky field shapefile, fly it in HITL, generate the coverage report.

---

## Phase 6 — Sprig Stack Integration (Weeks 17–20)

**Goal:** GCS is a node in the Sprig ecosystem, not a standalone tool.

Tasks:
- **Avilogix sync** — flight logs, maintenance events, anomaly flags pushed to Avilogix automatically post-flight (REST + auth)
- **CropIntel pull** — prescription maps and field boundaries pulled from CropIntel by field ID
- **Pilot identity** — login to Sprig SSO, all missions tagged with pilot
- **Aircraft identity** — tail number lookup against Avilogix, pre-flight verifies airworthiness status before allowing arm

**Deliverable:** End-to-end mission: pilot logs in, pulls prescription from CropIntel, plans mission, flies, logs auto-sync to Avilogix, maintenance event auto-created if any subsystem flagged a fault.

---

## Phase 7 — Polish, Mobile, Release (Weeks 21–24)

**Goal:** v1.0 — shippable to Sprig operators.

Tasks:
- Android tablet parity (primary field target)
- iOS parity (if economic)
- Regression matrix across Phase 0–6 deliverables on every platform
- User documentation (operator guide, not developer guide)
- Auto-update channel (separate from upstream QGC)
- Crash reporting (Sentry or equivalent)
- Code signing for all platforms

**Deliverable:** Avicontrol v2.0.0 tagged and released.

---

## Get-Weird Latitude (Stretch — pursue selectively, not all)

These are encouraged where they fit. Pick the ones that compound with the core build, ignore the ones that don't earn their keep:

- **Embedded X-Plane preview** — render a small X-Plane view inside fly view during HITL, synced to the same vehicle state, for pilot training
- **LLM mission planner** — "plan a 200-acre canola field with 60ft swath, racetrack turns, 8 mph ground speed, refill at the road on the south boundary" → generates the mission, pilot reviews and uploads
- **Predictive engine health** — surface anomaly scores on Rotax/MaxxECU telemetry trends, flag degradation before failure
- **Embedded knowledge graph viewer** — ship the Understand Anything graph of the *Sprig stack* (CM5 firmware + GCS + Avilogix schema) inside the GCS settings, navigable, with AI Q&A — meta but genuinely useful for ops
- **Web companion mode** — Qt WASM build for browser-based mission review (read-only initially)
- **In-GCS firmware updates** — Cube Orange+ PX4 + CM5 Rust binaries flashed from the GCS over the existing telemetry link
- **Voice annotations** — pilot taps to dictate notes mid-flight, transcribed and attached to the tlog
- **Multi-aircraft fleet view** — multiple S1000s on one map for coordinated ops on large fields

---

## Conventions

**Patch ledger format** — see `UPSTREAM_PATCHES.md` for the canonical template.

**ADR format** (`docs/decisions/NNN-title.md`): Context → Decision → Consequences. Short.

**Phase prompts:** Each phase gets a dedicated expanded prompt at `docs/prompts/phase-N.md` that picks up where this kickoff leaves off — don't try to one-shot the whole project from this file.

**Rebase cadence:** Weekly. Drift is the enemy. If a rebase takes more than a day, the patch ledger is too thick — refactor toward `custom/`.

---

## First Action

Stop reading. Open Claude Code at the repo root. Read `docs/architecture/00-overview.md`, then move to `docs/prompts/phase-1.md` to begin Phase 1.
