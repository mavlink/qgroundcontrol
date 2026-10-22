# CI Scripts

`tools/ci/` contains the Python entrypoints that GitHub Actions workflows and composite actions run
by file path (`python3 "${GITHUB_WORKSPACE}/tools/ci/<package>/<script>.py"`). Each subpackage
groups one CI concern; put reusable helpers with multiple consumers in
[`tools/common/`](../common/README.md) or [`tools/qgc_tools/`](../qgc_tools/README.md).

## Packages

| Package | Scope |
| --- | --- |
| `planning/` | Change detection and build-matrix planners |
| `cache/` | Compiler, moc, CPM, and GitHub Actions cache planning and maintenance |
| `runner/` | Runner host preparation and managed runner images |
| `compile/` | CMake and Docker build drivers with timing and compiler-cache reporting |
| `packaging/` | Locate, validate, and boot-test packaged build outputs |
| `android/` | Android SDK setup, build retry, and emulator smoke tests |
| `ios/` | iOS packaging and simulator smoke tests |
| `publish/` | Attestation, publishing, and release gating |
| `reporting/` | Build-results comments, size, duration, and coverage reports, and PR labels |
| `gstreamer/` | GStreamer SDK packaging and mirroring |
| `security/` | Dependency SBOM generation and vulnerability-scanner installation |

Subpackage names avoid `build*` and `release`: `.gitignore` ignores directories with those names,
and Ruff skips `build` directories.

## Script Index

| Script | Purpose |
| --- | --- |
| `planning/analysis_matrix.py` | Plan Code Analysis jobs: PR shards and scan scope, one cache-seeding job for master pushes, none when nothing compiled changed |
| `planning/android_matrix.py` | Emit the Android build matrix, running change detection and the release-tag gate in-process |
| `planning/detect_changes.py` | Decide which platform builds an event requires, including the release-tag gate |
| `planning/plan_docker_builds.py` | Generate Docker build matrices, running change detection and the release-tag gate in-process |
| `cache/build_cache_artifact.py` | Retain compiler/moc snapshots as short-lived workflow artifacts and restore them |
| `cache/cache_plan.py` | Resolve the cache action's save policy, scope, keys, ccache budget, compiler/moc/CPM environment, and the sccache pilot's selection |
| `cache/ccache_helper.py` | Install the pinned, checksum-verified ccache (and mold, plus sccache for the [sccache pilot](../../.github/ci-overview.md#sccache-pilot), on Linux), summarize ccache or sccache in CI, and gate PR cache saves |
| `cache/cpm_helper.py` | Fingerprint CPM dependencies and the GStreamer SDK config for `cache_plan.py`, and create and apply runner-image CPM seeds |
| `cache/gh_cache_cleanup.py` | List and optionally delete GitHub Actions caches (`cache-cleanup.yml` sets its flags through `CLEANUP_*` variables) |
| `runner/apt_install_retry.py` | Install apt packages with bounded retries, RunsOn's in-region EC2 mirror, and a runner-owned download cache |
| `runner/enable_kvm.py` | Open `/dev/kvm` to every user on a nested-virtualization runner |
| `runner/install_dependencies_helper.py` | Print the Linux package list and apply apt and dependency-cache fixups |
| `runner/install_multipass.py` | Install Multipass and wait for daemon readiness |
| `runner/runner_image.py` | Verify, promote, and retire QGC runner AMIs |
| `compile/cmake_helper.py` | Run CI configure, build, and CTest steps with standard arguments and timing |
| `compile/docker_helper.py` | Run Docker builds and report compiler identities, timings, and cache statistics |
| `packaging/find_artifact.py` | Find exactly one build artifact by glob pattern (fails on none or several), optionally copying it |
| `packaging/validate_native_package.py` | Validate native package identity, version, contents, and installed layout |
| `packaging/verify_executable.py` | Boot-test a built QGroundControl executable, AppImage, or DMG |
| `android/android_boot_test.py` | Run the emulator boot smoke test |
| `android/android_build_retry.py` | Retry an Android build after a known intermittent Qt deployment-settings JSON truncation |
| `android/android_collect_diagnostics.py` | Collect emulator, ADB, AVD, and GStreamer diagnostics |
| `android/android_sdk_helper.py` | Install Android SDK/NDK packages with retries, configure tool paths, and create the PR debug keystore |
| `ios/ios_boot_test.py` | Run the QGC smoke test in a disposable iOS simulator |
| `ios/ios_package.py` | Select signing profiles and validate/package iOS bundles |
| `publish/attest_helper.py` | Prepare artifacts for attestation (checksum sidecar, CPM dependency SBOM, build manifest); verify or write standalone checksums |
| `publish/aws_upload.py` | Publish artifacts to S3 `builds/<ref>/`, plus `latest/` and one CloudFront invalidation for release tags |
| `publish/deploy_docs.py` | Deploy generated documentation to an external Pages repository |
| `publish/release_assets.py` | Validate checksums and enumerate release packages and SBOMs |
| `publish/release_builds.py` | Dispatch and freeze exact release workflow run identities |
| `reporting/collect_build_status.py` | Collect platform and pre-commit status for a head SHA and gate build-results jobs until every platform workflow completed |
| `reporting/download_artifacts.py` | Download prefix-selected artifacts from matching completed workflow runs and record distributable artifact sizes |
| `reporting/generate_build_results_comment.py` | Render the consolidated PR build-results comment |
| `reporting/gh_pr_size_label.py` | Apply a pull request's `size/*` label from its changed lines |
| `reporting/report_context.py` | Resolve and recheck the current PR or default-branch reporting context |
| `reporting/size_analysis.py` | Analyze and report binary-size changes |
| `reporting/test_duration_report.py` | Report slow tests and duration regressions from JUnit XML |
| `reporting/verify_coverage_thresholds.py` | Enforce line and branch coverage thresholds (read from `CMakeCache.txt` with `--build-dir`) |
| `reporting/workflow_runs.py` | Library: validate, select, and group workflow runs and load cached run snapshots for the reporting scripts |
| `gstreamer/gstreamer_archive.py` | Package GStreamer builds and optionally upload them to S3 |
| `gstreamer/mirror_gstreamer.py` | Verify and mirror upstream GStreamer release artifacts to S3 |
| `security/generate_cpm_sbom.py` | Generate a CycloneDX or SPDX SBOM from CPM package metadata and the bundled GStreamer |
| `security/install_grype.py` | Install checksum-verified Grype release assets into the scan action tool cache |

The script index and one-to-one production test coverage are checked by
[`tools/tests/ci/contracts/test_scripts_docs.py`](../tests/ci/contracts/test_scripts_docs.py).

## Shared Code and Bootstrapping

Entrypoints bootstrap like `tools/setup/` scripts: insert the `tools/` directory into `sys.path`
with one bare statement, import `_bootstrap` for the CI debug hooks, then import from the module
that owns the symbol ([details](../common/README.md#bootstrapping-imports)). Scripts import each
other as package modules:

```python
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import write_github_output
from qgc_tools.release_tag_check import check_release_tag

from ci.planning.detect_changes import detect_builds
```

`ci` is the top import-linter layer: it may import the other `tools/` packages, and none of them may
import it. Its subpackages are independent of each other (only `publish/attest_helper.py` reuses
`security/generate_cpm_sbom.py`); move code that several of them need into `common` or `qgc_tools`.
Keep package `__init__.py` files import-free. Sparse-checkout jobs must include the
entrypoint, `tools/_bootstrap.py`, imported `ci` modules with their package `__init__.py` files, and
transitive `common` and `qgc_tools` modules; `test_bootstrap_sparse_checkout.py` verifies that
closure.

`runner/apt_install_retry.py`, `runner/enable_kvm.py`, `runner/install_multipass.py`,
`runner/runner_image.py`, `packaging/validate_native_package.py`, and `ios/ios_boot_test.py` are
standalone: they import only the standard library and skip the bootstrap. `runner-images.yml`
checks out `runner_image.py` by itself, so it must stay that way.

Scripts that run before `setup-python` use the runner's system interpreter and must import only the
standard library and stdlib-only `tools/` modules. `PRE_SETUP_PYTHON_SCRIPTS` in
[`tools/tests/test_stdlib_only.py`](../tests/test_stdlib_only.py) lists them; the test imports each
one under `python -I -S` and fails when a job without `setup-python` runs a script missing from the
list.

## Tests

Add `tools/tests/ci/test_<script>.py` with every new production script; every test there must match
a script. Tests of workflows, composite actions, presets, and deploy files go in
`tools/tests/ci/contracts/`. Load repository YAML through `tools/tests/_helpers.py`
(`load_workflow()`, `load_action()`, `load_yaml()`, `get_steps()`, `get_step()`): it parses each
file once with libyaml, keys workflow triggers under `"on"`, and returns a private copy. Build
temporary git repositories with the `git_repo` fixture and `_helpers.git()`, which ignore user and
system git configuration. Mark tests of Windows-specific code paths with
`pytestmark = pytest.mark.windows`; the Windows CI job runs `pytest -m windows tools/tests` (serial; xdist worker start-up costs more than it saves there).
Run the same suite as CI (`pytest -q -n auto tools/tests` in the locked `tools/.venv`):

```bash
just test-python
```

Tests run in parallel (pytest-xdist) and in random order (pytest-randomly, which also reseeds
`random`), so a test must not depend on another test's side effects. The run ends with
`Reproduce the test order with --randomly-seed=N`; replay that order serially with
`just test-python -n 0 --randomly-seed=N`, or run in file order with `-p no:randomly`.
`tools/tests/conftest.py` restores `os.environ` after each test. pytest-timeout fails any test that
runs longer than 60 s. The `test` group also provides:

- pytest-subprocess's `fp` fixture for sequences of faked commands (`fp.register()`); prefer it
  over patching `subprocess.run` when the code under test runs real commands.
- Hypothesis for property tests (`@given`); `conftest.py` disables its per-example deadline.
  Hypothesis keeps failing examples in the self-ignored `.hypothesis/` directory and replays them
  first on the next run.
- syrupy's `snapshot` fixture for rendered output; snapshots live in `__snapshots__/*.ambr` next
  to the test. After an intended change, refresh them with `--snapshot-update` and review the diff.
