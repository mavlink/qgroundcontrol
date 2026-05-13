# ADR-001 — Fork QGC instead of building a Flutter GCS from scratch

**Status:** Accepted, 2026-05-12.
**Owner:** stratten.

## Context

Sprig Aerospace needs a ground control station for the S1000 dodecacopter hybrid agricultural UAS (Cube Orange+ / PX4, CM5 mission computer, Rotax 912 ULS + AXM3 generator HV bus, plus vendor avionics on native CAN). The GCS must run on a field tablet (Android primary, Windows/macOS secondary), surface vendor-specific telemetry, plan ag-specific transect patterns (racetrack, teardrop, trombone, headland), and integrate with the broader Sprig stack (Avilogix logs, CropIntel prescription maps, Sprig SSO).

Two viable starting points were evaluated:

1. **Fork QGroundControl** — Qt 6 / QML, mature MAVLink stack, PX4 + ArduPilot support, existing mission planning, existing GStreamer video, existing parameter system, `custom-example/` template for downstream forks.
2. **Build a Flutter GCS from scratch** — clean modern UI framework, single codebase for mobile/desktop/web, no Qt licensing concerns, freedom to design ag-specific UX without fighting legacy patterns.

## Decision

**Fork QGroundControl.** Upstream remote is `mavlink/qgroundcontrol`. Branch `sprig/main` tracks upstream `main` with weekly rebase cadence. Sprig customizations live in `custom/` mirroring `custom-example/`, with deviations from that rule tracked in `UPSTREAM_PATCHES.md`.

## Consequences

**Accepted because:**

- **MAVLink, PX4, ArduPilot integration is the long pole.** Rebuilding parameter sync, mission upload/download round-trips, MAVLink v2 signing, log replay, link types (Serial/UDP/TCP/Bluetooth), tlog format, and the firmware abstraction (flight modes, guided mode, autostart-ID-to-airframe mapping) from scratch is years of work. QGC has it tested and field-proven.
- **GStreamer video pipeline with hardware zero-copy across D3D11/12, GL, DMABuf, IOSurface, AHardwareBuffer** is not a weekend project. QGC has it.
- **The customer-skin pattern in `custom-example/` is well-trodden** — confirmed in Phase 0 architecture survey. `QGCCorePlugin` substitution, QML URL interception, custom factory registration for `PX4FirmwarePlugin`/`PX4AutoPilotPlugin`, and CMake-level branding hooks let most Sprig-specific work live outside core files.
- **Operator familiarity.** Pilots flying QGC today can fly Sprig GCS tomorrow with rebranded chrome and additional ag panels. A Flutter rewrite starts from zero on operator muscle memory.
- **Upstream-able improvements compound.** Any non-Sprig-specific perf or bug fix becomes a PR back to upstream, reducing our carry over time. Flutter starts at 100% maintenance burden forever.

**Costs accepted:**

- **Qt + QML ergonomics.** QML is not Flutter. Touch ergonomics on Android, especially for the ag-specific UI we want, will require effort. We accept this in exchange for the value above.
- **Weekly rebase tax.** Drift against upstream is the structural risk. Mitigation: aggressive `custom/`-folder discipline, `UPSTREAM_PATCHES.md` ledger, weekly rebase as a hard cadence not a suggestion. If rebases regularly take more than a day, the patch ledger is too thick — that triggers a refactor toward `custom/` or upstream PRs, not a relaxation of the cadence.
- **Architectural seams that don't fit `custom/`.** Phase 0 survey identified three: (a) complex-mission-item registration in `MissionController.cc` requires source patches for new `TransectStyleComplexItem` subclasses; (b) `FlyView.qml`/`PlanView.qml` instantiate toolbar/right-panel chrome by type rather than via `Loader`, so adding toggleable layouts needs source patches; (c) link types are hardcoded enums (not currently on Sprig's roadmap). For each, we will propose an upstream extension-point PR alongside the local patch.
- **Mission-critical assumption: `NAMED_VALUE_FLOAT` is not auto-surfaced as Facts.** This was a quick-win path we'd hoped for; it doesn't exist in stock QGC. Sprig telemetry needs a proper MAVLink dialect (`sprig.xml` in a forked `mavlink/mavlink`) plus custom `FactGroup` subclasses. The dialect fork is CMake-controllable, so it doesn't add patch-ledger weight.

**Revisit when:**

- A rebase fails or takes >2 days even after refactoring the patch ledger toward `custom/`. That's a signal upstream has structurally diverged from us and the fork is becoming a maintenance liability.
- Qt LGPL terms change in a way that compromises Sprig's ability to ship.
- A second product line emerges where the QGC chrome is actively in the way (e.g., a fully autonomous mission console with no human-pilot operator) — at which point splitting that product off to a fresh stack may make sense, but the S1000 GCS stays on QGC.
