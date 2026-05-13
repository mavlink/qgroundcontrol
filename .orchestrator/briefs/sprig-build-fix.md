# Brief: Fix SprigCorePlugin build errors

## Context
QGroundControl Sprig fork (`custom/` overlay). The `custom/src/SprigCorePlugin.cc` defines two out-of-line member functions that have no matching declaration in `custom/src/SprigCorePlugin.h`, causing compilation to fail:

```
custom/src/SprigCorePlugin.cc:84:26: error: out-of-line definition of 'stableVersionCheckFileUrl' does not match any declaration in 'SprigCorePlugin'
custom/src/SprigCorePlugin.cc:89:26: error: out-of-line definition of 'stableDownloadLocation' does not match any declaration in 'SprigCorePlugin'
```

Both functions are virtuals on the base class `QGCCorePlugin` (declared at `src/API/QGCCorePlugin.h:128` and `:135`):

```cpp
virtual QString stableVersionCheckFileUrl() const { ... }
virtual QString stableDownloadLocation()    const { ... }
```

The intent in the Sprig plugin is to override both with placeholder Sprig URLs (Phase 7 will wire up real auto-update; see the `// Placeholder` comment in the .cc).

## Task
Add the two missing override declarations to `class SprigCorePlugin` in `custom/src/SprigCorePlugin.h`. They should mirror the existing override pattern in that class (look at the existing `void init() override;` / `void cleanup() final;` style — keep style consistent with what's already there).

Place them in the public "Overrides from QGCCorePlugin" section, near the other overrides. Use `override` (these are not leaf finals in the QGC codebase).

Signatures to declare:
```cpp
QString stableVersionCheckFileUrl() const override;
QString stableDownloadLocation()    const override;
```

You will likely need `#include <QtCore/QString>` — check whether QString is already transitively available via the existing includes; only add the include if the build still fails after adding the declarations.

## Verification (must pass before reporting done)
From repo root:
```
cmake --build build/Qt_6_10_3_for_macOS-Debug 2>&1 | tail -40
```
The build must complete without the two errors above. A successful target build (or progressing past `SprigCorePlugin.cc.o` without error) is sufficient evidence — the wider build may have other unrelated issues, but `SprigCorePlugin.cc.o` must compile cleanly.

## Constraints
- Only edit `custom/src/SprigCorePlugin.h` (and `.cc` only if absolutely required — it should not be).
- Do not modify base class headers.
- Do not change the placeholder URL strings.
- Follow project conventions in `AGENTS.md` / `CODING_STYLE.md`.
- Keep the diff minimal — two declarations.
