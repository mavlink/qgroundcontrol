# CI Scripts

`.github/scripts/` contains Python entrypoints used by GitHub Actions workflows and composite
actions. Keep workflow-facing entrypoints here so their paths remain stable; put reusable helpers
with multiple consumers in [`tools/common/`](../../tools/common/README.md).

See [`.github/README.md`](../README.md) for the surrounding workflow and action layout.

## Script Index

| Script | Area | Purpose |
| --- | --- | --- |
| `android_boot_test.py` | Android | Run the emulator boot smoke test |
| `android_build_retry.py` | Android | Retry known transient Qt deployment-settings and Gradle repository failures |
| `android_collect_diagnostics.py` | Android | Collect emulator, ADB, AVD, and GStreamer failure diagnostics |
| `android_sdk_helper.py` | Android | Resolve and configure Android SDK and NDK paths |
| `attest_helper.py` | Release | Gate SBOM signing and resolve artifact paths |
| `aws_upload.py` | Release | Validate and upload artifacts to AWS S3 |
| `cache_policy.py` | Cache | Resolve cache-save policy for the current workflow event |
| `ccache_helper.py` | Cache | Configure, install, and summarize ccache in CI |
| `ci_bootstrap.py` | Bootstrap | Make `tools/common` importable from CI entrypoints |
| `cmake_helper.py` | Build | Configure from repository presets and run CI build/test helpers |
| `collect_artifact_sizes.py` | Reporting | Collect artifact sizes from platform workflow runs |
| `collect_build_status.py` | Reporting | Collect platform and pre-commit status for PR comments |
| `coverage_comment.py` | Reporting | Build PR coverage comments from Cobertura XML |
| `cpm_helper.py` | Cache | Fingerprint CPM dependencies and configure their source cache |
| `deploy_docs.py` | Release | Deploy generated documentation to an external Pages repository |
| `detect_changes.py` | Planning | Decide which platform builds a change requires |
| `docker_helper.py` | Build | Provide Docker workflow build operations |
| `download_artifacts.py` | Artifacts | Download artifacts from matching completed workflow runs |
| `find_artifact.py` | Artifacts | Find optional or required build artifacts by glob pattern |
| `generate_build_results_comment.py` | Reporting | Render the consolidated PR build-results comment |
| `generate_cpm_sbom.py` | Security | Generate CycloneDX or SPDX SBOMs from CPM package metadata |
| `gh_cache_cleanup.py` | Cache | List and optionally delete GitHub Actions caches |
| `gh_pr_size_label.py` | Reporting | Read and prune pull-request `size/*` labels |
| `gstreamer_archive.py` | GStreamer | Package GStreamer builds and optionally upload them to S3 |
| `install_dependencies_helper.py` | Bootstrap | Apply Linux dependency-cache fixups before project dependencies exist |
| `linux_debug_matrix.py` | Planning | Emit the Linux debug-validation matrix |
| `mirror_gstreamer.py` | GStreamer | Verify and mirror upstream GStreamer release artifacts to S3 |
| `mold_helper.py` | Build | Install a pinned and checksum-verified mold linker |
| `plan_docker_builds.py` | Planning | Generate Docker build matrices from changed files |
| `precommit_results.py` | Reporting | Normalize pre-commit output into CI artifacts |
| `resolve_gstreamer_config.py` | GStreamer | Select the platform-specific GStreamer version |
| `size_analysis.py` | Reporting | Analyze and report binary-size changes |
| `test_duration_report.py` | Reporting | Report slow tests and duration regressions from JUnit XML |
| `validate_native_package.py` | Packaging | Validate native package identity, version, contents, and installed layout |
| `verify_coverage_thresholds.py` | Reporting | Enforce line and branch coverage thresholds |
| `verify_executable.py` | Build | Boot-test a built QGroundControl executable |

The script index and one-to-one production test coverage are checked by
`tests/test_scripts_docs.py`.

## Shared Code and Bootstrapping

Call `ensure_tools_dir` before importing shared helpers, then import from the module that owns the
symbol:

```python
from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import write_github_output
```

Do not add exports to `ci_bootstrap.py`; it is deliberately a small compatibility shim. Shared
behavior belongs in the narrowest suitable `tools/common` module. Keep
`install_dependencies_helper.py` and the dependency-install portions of `ccache_helper.py`
standard-library-only because they run before script dependencies are available.

Sparse-checkout jobs must include the entrypoint, `ci_bootstrap.py`, `tools/_bootstrap.py`, and the
transitive `common` modules. `test_bootstrap_sparse_checkout.py` verifies that import closure and
the complete build-results bootstrap payload.

## Tests

Add `tests/test_<script>.py` with every new production script. Prefer testing parsed inputs and
command construction without network access; patch only the external boundary.

Run the same combined suite as CI:

```bash
pytest -q tools/tests .github/scripts/tests
```
