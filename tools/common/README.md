# QGC Shared Tooling

`tools/common/` contains small, dependency-light helpers shared by QGC developer and CI scripts.
Import from the module that defines a helper; `common/__init__.py` intentionally does not re-export
symbols. Explicit imports keep runtime and sparse-checkout dependencies visible.

```python
from common.file_traversal import find_repo_root
from common.proc import run_captured
```

## Module Index

| Module | Purpose |
| ------ | ------- |
| `analysis_diagnostics.py` | analyze.py's `*-diagnostics.json` reports: each tool's unique warnings across shards, and those on changed lines |
| `aws.py` | Allowlisted public S3 object checks and uploads |
| `build_config.py` | `.github/build-config.json` lookup, validation, and CI export |
| `cmake.py` | CMake cache variable parsing |
| `coverage_summary.py` | gcovr JSON summary line, function, and branch coverage metrics |
| `file_traversal.py` | Repository-root discovery and filtered C++ file traversal |
| `format.py` | Human-readable byte, size-delta, count, duration, percentage, and ratio formatting, and one-line clipped names (`clip`) |
| `gh_actions.py` | GitHub CLI calls, read-only API retries and JSON reads, workflow artifact archive downloads, GitHub timestamps and durations, annotations (with optional `title`, `file`, `line`, and `col`), log groups, outputs, summaries, and CI detection |
| `git.py` | Captured Git commands, default-branch discovery, pull request diff refs, diff bases, and changed line ranges |
| `gstreamer.py` | GStreamer SDK package and mirror URLs and their pinned checksums |
| `io.py` | JSON/TOML I/O, checksums, atomic writes, and safe archive extraction |
| `junit.py` | CTest JUnit XML test cases (outcome, duration, failure excerpt) and suite wall time |
| `logging.py` | Color-aware terminal logging |
| `markdown.py` | GitHub-Flavored Markdown tables, code spans and blocks, `<details>` blocks, untrusted-text escaping, and the shared job-summary section layout (`summary_section`) |
| `net.py` | Dependency-free atomic downloads and small fetches with optional SHA-256 verification; retries only transient failures (connection errors, timeouts, 5xx, 408, 429), so a 404 fails at once; `fetch_release_member` extracts one member from a pinned release archive |
| `paths.py` | `TOOLS_DIR` and `REPO_ROOT`, resolved from the module location |
| `platform.py` | OS and CPU architecture normalization |
| `proc.py` | Captured, tee (through Bash, or a live-flushed Python pipe), and bounded-retry subprocess execution (optionally killing a timed-out attempt's whole process tree, `kill_process_tree`; the final failure logs the command and the end of its captured stderr), bytes-or-text output decoding, `require_tools` (`MissingToolError` names every tool missing from `PATH`), and deadline polling with `wait_until` |
| `qt.py` | Qt installer-kit discovery for host tools such as `qt-cmake` and `lupdate` |
| `tool_version.py` | External-tool and `uv.lock` version lookup |
| `xml.py` | Safe XML parsing with entity-declaration rejection |

API behavior and edge cases are covered by matching files under `tools/tests/`, such as
`test_proc.py`, `test_io.py`, and `test_gh_actions.py`.

## Bootstrapping Imports

Top-level `tools/*.py` scripts run with `tools/` as `sys.path[0]` and import `common` directly.
Entry scripts in subdirectories (`tools/setup/`, `tools/ci/<package>/`, ...) first insert `tools/`
with one bare statement, which Ruff's import-position rule (E402) accepts:

```python
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))  # tools/ci/<package>/x.py

import _bootstrap as _bootstrap
from common.proc import run_captured
```

`import _bootstrap as _bootstrap` installs the CI debug hooks (see
[`tools/README.md`](../README.md#python-script-conventions)); the redundant alias marks the unused
import as intentional. Library modules without a `__main__` block do not touch `sys.path`.
`tools/tests/test_bootstrap.py` enforces this shape.

CI jobs that use sparse checkout must include the entrypoint, `tools/_bootstrap.py`, and transitive
`ci`, `common`, and `qgc_tools` modules. `test_bootstrap_sparse_checkout.py` checks that closure
automatically.

## Adding or Reusing Helpers

Before adding a helper, search this directory and its call sites. Add shared code only when it has
multiple consumers or centralizes a correctness boundary such as safe extraction, retries, GitHub
output encoding, or platform normalization.

When a new helper is warranted:

1. Put it in the narrowest existing module; create a module only for a distinct concern.
2. Keep imports dependency-light and defer optional packages until the function that needs them.
3. Export the supported surface through the module's `__all__` when it has one.
4. Add focused tests under `tools/tests/`.
5. Run `just test-python` and `just lint` (Ruff, Pyright, and import-linter run as pre-commit hooks).
6. Update sparse-checkout lists if the automatic policy test reports a missing module.
