# Upstream Patch Ledger

Every change to a file outside `custom/` requires an entry here. The ledger exists so that weekly rebases against `mavlink/qgroundcontrol` main can be validated mechanically: each patch is re-applied, re-tested, and the "Last verified clean" date updated.

If a patch becomes painful to re-apply, that's a signal to either (a) refactor the change into `custom/`, or (b) propose an upstream extension point that obsoletes the patch.

## Format

```
## PATCH-NNN — <short title>
- **Files:** <relative paths>
- **Reason:** <why this can't live in custom/>
- **Upstream PR:** <#NNNNN, status, or "not yet drafted">
- **Rebase risk:** <Low / Medium / High>  <one-line why>
- **Owner:** <github handle>
- **Introduced:** <YYYY-MM-DD> against upstream main @ <sha>
- **Last verified clean:** <YYYY-MM-DD> against upstream main @ <sha>
- **Notes:** <optional context>
```

---

## Active patches

*None yet.* Phase 0 is documentation-only. Phase 1 (branding) is designed to land zero entries here.

## Anticipated patches (Phase 2 onward — for planning only, not yet applied)

These are not real entries — they're the patches we expect to need based on the Phase 0 architecture survey. Convert to a real entry when actually written.

- **PATCH-001 (Phase 3):** Register Sprig `TransectStyleComplexItem` subclasses in `MissionController::insertComplexMissionItem()` (`src/MissionManager/MissionController.cc:394–413`) and `MissionController::_loadJsonFile()` complex-item JSON dispatch (`:717–755`). High rebase risk — central file. Upstream PR target: a factory-based registry so downstream forks can register subclasses via `CorePlugin`.

- **PATCH-002 / -003 (Phase 2, conditional):** Replace hardcoded toolbar instantiation in `src/FlyView/FlyView.qml:174–178` and `src/PlanView/PlanView.qml:228–232` with `Loader`-based chrome. Only needed if Sprig wants operator-toggleable layouts; otherwise the URL interceptor in `custom/` can shadow the whole QML file. Upstream PR target: same `Loader` change, generally useful for any downstream fork.
