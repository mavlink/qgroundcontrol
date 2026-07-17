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
| `analyzer.py` | Analyzer result types, base class, and ordered parallel execution |
| `artifact_metadata.py` | Validated GitHub Actions artifact name and size interchange |
| `aws.py` | Allowlisted public S3 object checks and uploads |
| `build_config.py` | `.github/build-config.json` lookup, validation, and CI export |
| `cmake.py` | CMake cache variable parsing |
| `cobertura.py` | Cobertura line and branch coverage metrics |
| `deps.py` | External-tool checks and project-aware Python package installation |
| `env.py` | CI environment detection |
| `errors.py` | Shared tooling exceptions |
| `file_traversal.py` | Repository-root discovery and filtered C++ file traversal |
| `format.py` | Human-readable byte and size-delta formatting |
| `gh_actions.py` | GitHub CLI calls, annotations, outputs, environment, and step summaries |
| `git.py` | Captured Git commands and default-branch discovery |
| `github_runs.py` | Workflow-run loading, filtering, and latest-run selection |
| `io.py` | JSON/TOML I/O, checksums, atomic writes, and safe archive extraction |
| `logging.py` | Color-aware terminal logging |
| `markdown.py` | Escaped GitHub-Flavored Markdown tables |
| `net.py` | Dependency-free downloads and retry policies |
| `opener.py` | Cross-platform default-application launching |
| `patterns.py` | QGC-specific source-analysis regular expressions |
| `platform.py` | OS and CPU architecture normalization |
| `proc.py` | Captured text and byte subprocess execution |
| `tool_version.py` | External-tool and `uv.lock` version lookup |
| `xml.py` | Safe XML parsing with entity-declaration rejection |
| `shell-utils.sh` | Shared shell logging for developer scripts |

API behavior and edge cases are covered by matching files under `tools/tests/`, such as
`test_proc.py`, `test_io.py`, and `test_gh_actions.py`.

## Bootstrapping Imports

Scripts under `tools/` must initialize the tools path before importing `common`:

```python
from _bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.proc import run_captured
```

Scripts under `.github/scripts/` use the CI shim instead:

```python
from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import write_github_output
```

The import after `ensure_tools_dir` is intentionally separated and may need `# noqa: E402` in
files covered by Ruff's import-position rule. Do not manually modify `sys.path` in new scripts.

CI jobs that use sparse checkout must include the entrypoint, bootstrap shim, and transitive
`common` modules. `test_bootstrap_sparse_checkout.py` checks that closure automatically.

## Adding or Reusing Helpers

Before adding a helper, search this directory and its call sites. Add shared code only when it has
multiple consumers or centralizes a correctness boundary such as safe extraction, retries, GitHub
output encoding, or platform normalization.

When a new helper is warranted:

1. Put it in the narrowest existing module; create a module only for a distinct concern.
2. Keep imports dependency-light and defer optional packages until the function that needs them.
3. Export the supported surface through the module's `__all__` when it has one.
4. Add focused tests under `tools/tests/`.
5. Run `pytest -q tools/tests .github/scripts/tests`, Ruff, Pyright, and import-linter.
6. Update sparse-checkout lists if the automatic policy test reports a missing module.

Standalone packages under `tools/skills/` are copied and run outside the repository, so their
reference scripts must not depend on `tools/common/`.
