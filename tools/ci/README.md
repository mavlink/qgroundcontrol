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
| `android/` | Android SDK setup, build retry, lint reporting, and emulator smoke tests |
| `ios/` | iOS packaging and simulator smoke tests |
| `publish/` | Attestation, publishing, release gating, draft release notes, and the GitHub Pages site |
| `reporting/` | Build-results comments, job-summary Setup and test sections, size and coverage reports, and PR labels |
| `gstreamer/` | GStreamer SDK packaging and mirroring |
| `security/` | Dependency SBOM generation and vulnerability-scanner installation |

Subpackage names avoid `build*` and `release`: `.gitignore` ignores directories with those names,
and Ruff skips `build` directories.

## Script Index

| Script | Purpose |
| --- | --- |
| `planning/analysis_matrix.py` | Plan Code Analysis jobs (configure and analyzer arguments per tool family): PR shards and scan scope, one cache-seeding job for master pushes, none when nothing compiled changed |
| `planning/android_matrix.py` | Library: the Android build legs (hosts, runners, ABIs) for `plan_platform.py` |
| `planning/detect_changes.py` | Library: decide which platforms an event builds, including the release-tag gate |
| `planning/plan_docker_builds.py` | Library: select the Docker variants an event builds, each with the image push target |
| `planning/plan_platform.py` | `_detect-changes.yml`'s planner: change detection and the release-tag gate once, then the Android or Docker matrix |
| `cache/build_cache_artifact.py` | Retain compiler/moc snapshots as short-lived workflow artifacts and restore them |
| `cache/cache_backend.py` | Detect whether RunsOn's S3 proxy serves the Actions cache and export `QGC_ACTIONS_CACHE_BACKEND` for `setup-python` |
| `cache/cache_plan.py` | Resolve the cache action's save policy, scope, keys, ccache budget, compiler/moc/CPM environment, and the sccache pilot's selection |
| `cache/ccache_helper.py` | Install the pinned, checksum-verified ccache (and mold, plus sccache for the [sccache pilot](../../.github/ci-overview.md#sccache-pilot), on Linux) and plan `save-build-cache`: gate PR saves, pack the snapshot artifact, and choose the cache keys |
| `cache/cpm_helper.py` | Fingerprint CPM dependencies and the GStreamer SDK config for `cache_plan.py`, and create and apply runner-image CPM seeds |
| `cache/gh_cache_cleanup.py` | List and optionally delete GitHub Actions caches, taking the branch and mode from `cache-cleanup.yml`'s triggering event (`CLEANUP_*` variables override them) |
| `runner/analysis_tools.py` | Install the clang-tidy wheel hash-pinned in `build-config.json`, verify `.clang-tidy`, and put LLVM, clang-tidy, and Clazy on `PATH` for `analysis.yml` |
| `runner/apt_install_retry.py` | Install apt packages with bounded retries, RunsOn's in-region EC2 mirror, and a runner-owned download cache |
| `runner/build_setup.py` | Validate `build-setup`'s inputs and resolve the values its steps read: Qt hosts, architectures and modules, cache target, Xcode and MSVC selections, Windows ARM64 cross builds' host Qt, Android ABIs and SDK environment, and the `build-config.json` tool versions |
| `runner/enable_kvm.py` | Open `/dev/kvm` to every user on a nested-virtualization runner |
| `runner/install_dependencies_helper.py` | `install-dependencies` entry points: plan the Linux apt cache key, install the apt packages or the macOS Homebrew packages and GStreamer (both without the tools CI pins elsewhere), and install Windows MSVC and NSIS (macOS and Windows in three timed attempts that kill a hung attempt's process tree) |
| `runner/install_multipass.py` | Install Multipass and wait for daemon readiness; with `--allow-bridge-forwarding`, undo Docker's iptables `FORWARD` drop policy so guests reach the network |
| `runner/runner_image.py` | Check `runner-images.yml`'s branch and AWS settings, then output, verify, promote, and retire QGC runner AMIs |
| `runner/validate_runner_configs.py` | `packer-validate` and `runs-on-lint` hooks: validate the runner image Packer template and lint the RunsOn configuration with the Packer and RunsOn CLI pinned in `build-config.json` |
| `compile/cmake_helper.py` | Run CI configure, build, and CTest steps with standard arguments and timing, tee configure and build output to logs, and write the job summary's Configure (result, duration, preset, generator, configure-check reuse) and Build (result, duration, toolchain, this build's compiler and moc cache hit rates, slowest build steps) sections; after a failure, annotate and summarize the first CMake error or the failed targets and first build errors |
| `compile/docker_helper.py` | Run Docker builds and repository scripts in builder images, and report compiler identities, timings, and cache statistics |
| `packaging/artifact_link.py` | Link an uploaded diagnostics artifact (upload-artifact's `artifact-url`) from the job summary |
| `packaging/find_artifact.py` | Find exactly one build artifact by glob pattern (fails on none or several), optionally copying it |
| `packaging/validate_native_package.py` | Validate native package identity, version, contents, and installed layout |
| `packaging/verify_executable.py` | Boot-test a built QGroundControl executable, AppImage, or DMG; keep a failed boot's output, stacks, crash reports, and minidumps for the diagnostics artifact |
| `android/android_boot_test.py` | Run the emulator boot smoke test |
| `android/android_build_retry.py` | Retry an Android build after a known intermittent Qt deployment-settings JSON truncation |
| `android/android_collect_diagnostics.py` | Collect emulator, ADB, AVD, and GStreamer diagnostics |
| `android/android_lint.py` | Run `gradlew lintRelease` in the Gradle project androiddeployqt generated, annotate the first Error and Fatal issues from lint's XML report at their `android/` sources (else Gradle's errors), and write the job summary's Android Lint section (result, duration, error and warning counts, first errors); exits with Gradle's status |
| `android/android_sdk_helper.py` | Install the `build-config.json` Android SDK/NDK packages with retries, configure tool paths, and create the PR debug keystore |
| `ios/ios_boot_test.py` | Run the QGC smoke test in a disposable iOS simulator within a 25-minute budget; save each failed attempt's simulator and CoreSimulator logs and crash reports, annotate the failing phase and attempt, and write the Boot test summary |
| `ios/ios_package.py` | Select signing profiles and validate/package iOS bundles |
| `publish/attest_helper.py` | Prepare artifacts for attestation (checksum sidecar, CPM dependency SBOM, build manifest) and decide the AWS publish; verify or write standalone checksums |
| `publish/aws_upload.py` | Publish artifacts to S3 `builds/<ref>/` (the runner's ref by default), plus `latest/` and one CloudFront invalidation for release tags |
| `publish/deploy_docs.py` | Deploy generated documentation to an external Pages repository |
| `publish/pages_history.py` | Library: the Pages dashboard's history of master push builds: fetch the deployed `data/history.json` (any failure but a 404 stops the build, and so does a 404 once a `pages.yml` run has succeeded), collect each new run's result, wall time, Build-step durations, package sizes, JUnit suites and failed tests, `src/` line coverage, and unique Code Analysis warnings, newest first until the 15-minute collection deadline, with two 120-second attempts per artifact, and merge by commit and workflow within the 1,000-commit and 365-day caps |
| `publish/pages_render.py` | Library: the Pages site's self-contained HTML pages and inline SVG charts (no scripts or external assets), each chart followed by a table of its latest values and linking every run |
| `publish/pages_site.py` | Build the GitHub Pages site for `pages.yml`: the CI health dashboard, `schemas/` (each `$id` must be its site URL), `downloads/` of the S3 master packages with their SHA-256 sidecars and provenance commits, and `api/` from the latest master Doxygen artifact or a link to api.qgroundcontrol.com (also when its download stalls); `--new-history` (`PAGES_NEW_HISTORY`) starts a new history when the deployed one is gone; checks its pages' internal links and writes the job summary's Pages site section |
| `publish/release_branch_check.py` | Stop `release.yml` before it drafts a release for a tag that no `origin/Stable*` branch contains |
| `publish/release_notes.py` | Draft a tag's release notes with git-cliff (`tools/configs/cliff.toml`) and create its draft GitHub Release unless one exists |
| `reporting/collect_build_status.py` | Collect platform, pre-commit, Code Analysis, and every other workflow's status for a head SHA (with `--status-out`, each run's state, attempt, duration and total wall time, and failed-step links and lost runners from its jobs) and save the latest runs for `download_artifacts.py` |
| `reporting/download_artifacts.py` | Download prefix-selected artifacts from matching completed workflow runs into one directory each, with a manifest naming each artifact's workflow and whether it downloaded, and record each workflow's latest run and its distributable artifacts (size, id, expiry) |
| `reporting/generate_build_results_comment.py` | Render the consolidated PR build-results comment: verdict, per-workflow table, other checks, the PR base's builds, failures, and collapsed tests, coverage, artifact downloads, pre-commit, and Code Analysis details, compared with the base's own builds |
| `reporting/gh_pr_size_label.py` | Apply a pull request's `size/*` label from its changed lines |
| `reporting/junit_report.py` | Summarize JUnit results (counts, failures, slowest tests) in the job summary, annotate failed tests and, optionally, slow tests (parsed with `common/junit.py`, like the build-results comment and the Pages dashboard) |
| `reporting/report_context.py` | Resolve a pull_request build's PR and base commit, and with `--comment` recheck that the build is current and create or update the PR's build-results comment |
| `reporting/setup_summary.py` | Write the job summary's Setup section for `build-setup`, also after a failed setup step, which it names: runner, image, RunsOn instance, CPUs and RAM, Qt (version, installer, source), CMake, compiler, Xcode, JDK, Android NDK, Python groups, and each build cache's restore result |
| `reporting/size_analysis.py` | Analyze and report binary-size changes |
| `reporting/verify_coverage_thresholds.py` | Enforce line and branch coverage thresholds (read from `CMakeCache.txt` with `--build-dir`) |
| `reporting/workflow_runs.py` | Library: read the triggering `workflow_run` payload and the configured platform workflows, which default the reporting scripts' run-query arguments; validate, select, and group workflow runs; load run snapshots |
| `gstreamer/gstreamer_archive.py` | Package GStreamer builds and optionally upload them to S3 |
| `gstreamer/mirror_gstreamer.py` | Verify and mirror upstream GStreamer release artifacts to S3 (`mirror-gstreamer.yml` sets its options through `MIRROR_*` variables) |
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

`cache/cache_backend.py`, `runner/apt_install_retry.py`, `runner/enable_kvm.py`,
`runner/install_multipass.py`, `runner/runner_image.py`, `packaging/validate_native_package.py`,
and `ios/ios_boot_test.py` are standalone: they import only the standard library and skip the
bootstrap. `runner-images.yml` checks out `runner_image.py` by itself, so it must stay that way.

Scripts that run before `setup-python` use the runner's system interpreter and must import only the
standard library and stdlib-only `tools/` modules. `PRE_SETUP_PYTHON_SCRIPTS` in
[`tools/tests/test_stdlib_only.py`](../tests/test_stdlib_only.py) lists them; the test imports each
one under `python -I -S` and fails when a job without `setup-python` runs a script missing from the
list. `setup-python` puts `tools/.venv` first on `PATH`; on Windows `install_python.py` also copies
the venv's `python.exe` to `python3.exe`, so later `python3` steps run in the venv on every OS.
As a fallback for a runner whose `PATH` still prefers its own interpreter, the test also holds every
script that a composite action runs as `python3` to the same rule; `_VENV_ACTION_SCRIPTS` exempts
the few that need third-party packages.

## Tests

Add `tools/tests/ci/test_<script>.py` with every new production script; every test there must match
a script. Entry scripts define `main(argv: list[str] | None = None) -> int`
(`tools/tests/test_bootstrap.py` checks the signature), so tests call `main([...])` instead of
patching `sys.argv`. Tests of workflows, composite actions, presets, and deploy files go in
`tools/tests/ci/contracts/`. Load repository YAML through `tools/tests/_helpers.py`
(`load_workflow()`, `load_action()`, `load_yaml()`, `get_steps()`, `get_step()`): it parses each
file once with libyaml, keys workflow triggers under `"on"`, and returns a private copy;
`workflow_names()` and `action_names()` enumerate them. Build temporary git repositories with the
`git_repo` fixture, `_helpers.git()` and `commit_files()`, which ignore user and system git
configuration. Point `GITHUB_OUTPUT`, `GITHUB_ENV`, `GITHUB_PATH` and `GITHUB_STEP_SUMMARY` at
empty temporary files with the `gh_output`, `gh_env`, `gh_path` and `gh_summary` fixtures, and
parse what a script wrote with `read_github_file()`, which also reads `name<<DELIMITER` blocks.
`workflow_run_event(**fields)` points `GITHUB_EVENT_PATH` at a `workflow_run` payload.
Import scripts outside the `tools/` packages with `load_script()`. A test reads the repository files
it checks directly, without skipping when they are missing: a file absent from a CI sparse checkout
fails the test. Mark the tests of each script that `windows.yml` runs on Windows, and of other
Windows-specific code paths, with `pytestmark = pytest.mark.windows`; the Windows CI job runs
`pytest -n 2 --durations=15 -m windows tools/tests` (two xdist workers are a trial; revert to
serial if CI shows it slower).
Run the same suite as CI (`pytest -q -n auto tools/tests` in the locked `tools/.venv`):

```bash
just test-python
```

Tests run in parallel (pytest-xdist) and in random order (pytest-randomly, which also reseeds
`random`), so a test must not depend on another test's side effects. The run ends with
`Reproduce the test order with --randomly-seed=N`; replay that order serially with
`just test-python -n 0 --randomly-seed=N`, or run in file order with `-p no:randomly`.
`tools/tests/conftest.py` restores `os.environ` after each test, and for the whole session removes
the runner's context (`CI`, `GITHUB_*`, `RUNNER_*`, `ACTIONS_*`, `RUNS_ON_*`, `QGC_CI_DEBUG`,
`QGC_ACTIONS_CACHE_BACKEND`), so tests behave the same in CI as locally and never write to the
job's real output or summary files; a test that needs one of these variables sets it.
pytest-timeout fails any test that runs longer than 60 s. The `test` group also provides:

- pytest-subprocess's `fp` fixture for sequences of faked commands (`fp.register()`); prefer it
  over patching `subprocess.run` when the code under test runs real commands.
- Hypothesis for property tests (`@given`); `conftest.py` disables its per-example deadline.
  Hypothesis keeps failing examples in `.cache/hypothesis/` (set in `conftest.py`) and replays them
  first on the next run.
- syrupy's `snapshot` fixture for rendered output; snapshots live in `__snapshots__/*.ambr` next
  to the test. After an intended change, refresh them with `--snapshot-update` and review the diff.
