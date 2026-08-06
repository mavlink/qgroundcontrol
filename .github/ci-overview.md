# `.github/` — CI, Workflows, and Repo Metadata

> See [`AGENTS.md`](../AGENTS.md) for the canonical agent guide (build/test/lint commands, coding
> conventions). This doc covers CI layout: workflows, composite actions, Python helpers, and
> build-config.

Platform workflows (`linux.yml`, `macos.yml`, `windows.yml`, `android.yml`, `ios.yml`) share logic
via composite actions and reusable workflows. Python helpers in `scripts/` are invoked by both.

## Contents

- [Layout](#layout)
- [Workflows](#workflows)
- [Composite Actions](#composite-actions)
- [Scripts](#scripts)
- [Build Configuration](#build-configuration)
- [Dependency Management](#dependency-management)
- [CI Conventions](#ci-conventions)
- [Tests](#tests)

## Layout

```text
.github/
├── workflows/                 # Platform builds, reusable workflows, and repo automation
├── actions/                   # Composite actions shared across workflows
├── scripts/                   # Python helpers invoked by workflows and actions
│   ├── templates/             # Jinja2 templates (build_results.md.j2)
│   └── tests/                 # pytest suite for scripts/ (see #tests)
├── build-config.json          # Centralized version numbers and build settings
├── build-config.schema.json   # JSON Schema for build-config.json
├── dependabot.yml             # Dependabot config (GitHub Actions only)
└── renovate.json              # Renovate config (npm, python, pre-commit)
```

## Workflows

| Workflow | Purpose |
| --- | --- |
| `linux.yml`, `macos.yml`, `windows.yml` | Desktop build + test |
| `android.yml`, `ios.yml` | Mobile builds |
| `_detect-changes.yml` | Reusable: skip builds on unrelated PRs |
| `build-results.yml` | Aggregate PR comment (`workflow_run` trigger) |
| `build-gstreamer.yml` | GStreamer SDK builds |
| `build-profile.yml` | CMake build profiling |
| `custom-build.yml` | Custom build validation |
| `docker.yml` | Docker image builds |
| `pre-commit.yml` | Linting and formatting checks |
| `check-links.yml` | Markdown link validation |
| `ci-scripts.yml` | Lints workflows (actionlint) and runs the CI Python script tests (see [Tests](#tests)) |
| `analysis.yml` | Static analysis |
| `codeql.yml` | CodeQL security scanning |
| `pr-checks.yml` | PR validation checks |
| `release.yml` | Release automation |
| `docs.yml`, `doxygen.yml` | Documentation deployment |
| `cache-cleanup.yml`, `cache-cleanup-pr.yml`, `_cache-cleanup.yml` | Cache maintenance (reusable + scheduled + PR-triggered) |
| `crowdin.yml`, `lupdate.yml` | Translation workflows |
| `dependency-review.yml` | Dependency security review |
| `scorecard.yml` | OpenSSF Scorecard |
| `flatpak.yml` | Flatpak builds |
| `mirror-gstreamer.yml` | Mirror upstream GStreamer releases to the QGC S3 bucket |
| `px4-metadata.yml` | PX4 metadata sync |
| `vm-builds.yml` | VM-based builds |
| `welcome.yml` | New contributor welcome |

## Composite Actions

| Action | Purpose |
| --- | --- |
| `cmake-configure` | Configure QGroundControl build with common options |
| `cmake-build` | Build QGroundControl with consistent options (timing, reviewdog, ccache) |
| `cmake-install` | Run `cmake --install` with a consistent config selector |
| `run-unit-tests` | Run unit tests via CTest and generate standardized test artifacts |
| `detect-changes` | Detect source, test, and CI changes for one or more platforms |
| `attest-and-upload` | Generate SBOM attestation and upload build artifact (GitHub + optional AWS) |
| `attest-sbom` | Generate SBOM and attest build provenance |
| `aws-credentials` | Configure AWS credentials via OIDC (preferred) or static keys |
| `aws-upload` | Upload release artifacts to AWS S3 and invalidate CloudFront cache |
| `build-config` | Read build toolchain versions from `.github/build-config.json` |
| `build-prerequisites` | Shared CI prerequisites for platform builds (build config, disk cleanup, CMake, Python) |
| `build-setup` | Common build environment setup: `build-prerequisites` + the mode-specific Qt installer |
| `build-action` | Unified build action |
| `build-results-bootstrap` | Sparse-checkout scripts/actions consumed by `build-results.yml` jobs |
| `cache` | Caching helpers |
| `cache-cleanup` | List and optionally delete GitHub Actions caches |
| `collect-artifact-sizes` | Query artifact sizes from GitHub API for all platform workflow runs |
| `coverage` | Generate and upload code coverage reports |
| `deploy-docs` | Deploy built docs to an external GitHub Pages repository |
| `docker` | Build QGC using Docker |
| `download-all-artifacts` | Download artifacts from all completed platform workflow runs for the same commit |
| `free-disk-space` | Wrapper pinning QGC's shared defaults over `endersonmenezes/free-disk-space` |
| `gate-platform-workflows` | Check that every platform workflow has completed for a given head SHA |
| `install-dependencies` | Platform dependency installation (GStreamer, etc.) |
| `android-emulator-test` | APK install + launch smoke test against an x86_64 Android emulator |
| `playstore` | Upload Android APK to Google Play Store |
| `qt-install` | Install Qt via aqtinstall with caching |
| `qt-android`, `qt-ios` | Mobile Qt setup |
| `replace-cache-entry` | Delete a stale GitHub Actions cache entry, then save the file at the same key |
| `setup-python` | Python + uv + dependency installation |
| `size-analysis` | Binary size tracking (bloaty) |
| `test-duration-report` | Analyze JUnit test durations and summarize slow tests |
| `test-report` | Publish and upload test results |
| `verify-executable` | Post-build executable boot-test verification |

## Scripts

Python helpers in `.github/scripts/` invoked by workflows and composite actions.

| Script | Purpose |
| --- | --- |
| `android_boot_test.py` | Android emulator boot smoke test |
| `android_build_retry.py` | Retry an Android build after a known intermittent Qt deployment-settings failure |
| `android_collect_diagnostics.py` | Collect emulator failure diagnostics (build, adb dumps, GStreamer error grep, AVD logs) |
| `android_sdk_helper.py` | Android SDK/NDK setup helpers |
| `attest_helper.py` | Gate SBOM signing and resolve artifact paths |
| `aws_upload.py` | Validate and upload artifacts to AWS S3 |
| `cache_policy.py` | Resolve the cache save policy for the current workflow event |
| `ccache_helper.py` | Ccache CI helper: config output, binary install, build summary |
| `check_baseline_ready.py` | Verify baseline-cache update readiness for a commit SHA |
| `ci_bootstrap.py` | Bootstrap helper that makes `tools/common` imports work for CI scripts |
| `cmake_helper.py` | CMake build and configure helpers |
| `collect_artifact_sizes.py` | Collect artifact sizes for latest successful platform workflow runs |
| `collect_build_status.py` | Collect latest platform/pre-commit status for build-results comments |
| `coverage_comment.py` | Build coverage report comments from Cobertura XML |
| `cpm_helper.py` | CPM CI helper: dependency fingerprint, source cache configuration |
| `deploy_docs.py` | Deploy built docs to an external GitHub Pages repository |
| `detect_changes.py` | Detect whether a CI build is needed based on changed files and platform |
| `docker_helper.py` | Docker build helpers |
| `download_artifacts.py` | Download build artifacts from completed platform workflow runs |
| `find_artifact.py` | Find build artifacts by glob pattern in a directory |
| `generate_build_results_comment.py` | Generate consolidated PR build-results comment |
| `generate_cpm_sbom.py` | Generate a CycloneDX SBOM from a CMake build directory's CPM package metadata |
| `gh_cache_cleanup.py` | List and optionally delete GitHub Actions caches via `gh-actions-cache` |
| `gh_pr_size_label.py` | Read and prune `size/*` labels on a pull request |
| `gstreamer_archive.py` | Package GStreamer builds and optionally upload to S3 |
| `install_dependencies_helper.py` | Post-install fixups for CI dependency caching on Linux |
| `linux_debug_matrix.py` | Emit the `linux.yml` debug-validation matrix as a JSON `include` list |
| `mirror_gstreamer.py` | Mirror official upstream GStreamer release artifacts to the QGC S3 bucket |
| `mold_helper.py` | Download and install a pinned, SHA256-verified `mold` linker binary (Linux) |
| `plan_docker_builds.py` | Generate Docker workflow build matrices from changed files |
| `precommit_results.py` | Normalize pre-commit outputs into uploaded CI artifacts |
| `resolve_gstreamer_config.py` | Pick the platform-specific GStreamer version from build-config outputs |
| `size_analysis.py` | Analyze binary size changes |
| `test_duration_report.py` | Generate test-duration reports and regressions |
| `verify_coverage_thresholds.py` | Verify `coverage.xml` meets line and branch coverage thresholds |
| `verify_executable.py` | Verify the QGroundControl executable with a boot test |
| `xml_utils.py` | Safe XML parsing via `defusedxml` |

## Build Configuration

- **`build-config.json`**: Centralized version numbers (Qt, Android SDK/NDK, Apple/Xcode, CMake
  minimum, GStreamer) and build settings, validated against `build-config.schema.json`.
- Read values via `common.build_config.get_build_config_value()`, or via the `build-config`
  composite action from within a workflow step.

## Dependency Management

Dependency updates are split between two bots to avoid overlapping PRs:

- **Dependabot** (`.github/dependabot.yml`) owns `github-actions` updates only, grouped weekly.
  Merge with `@dependabot merge`.
- **Renovate** (`.github/renovate.json`) owns `npm`, `python` (pep621/uv), and `pre-commit`
  updates, grouped into a single weekly PR. GitHub Actions paths are excluded via `ignorePaths`.

## CI Conventions

- **Dependencies**: CI Python scripts use `httpx` for GitHub API access and `jinja2` for
  templating. Deps managed in `tools/pyproject.toml` under `[project.optional-dependencies] scripts`.
- **Shared helpers**: `gh_actions.py` provides GitHub API pagination (httpx) with `gh` CLI
  fallback. Import as `from common.gh_actions import ...`.
- **Bootstrap scripts** (`install_dependencies_helper.py`, `ccache_helper.py`): use stdlib only —
  they run before dependencies are installed.
- **Outputs**: use `common.gh_actions.write_github_output()` for `$GITHUB_OUTPUT` writes.

## Tests

`ci-scripts.yml` runs two jobs on changes under `.github/` and `tools/`: `actionlint` (workflow
linting) and a pytest job covering both `tools/tests` and `.github/scripts/tests`.

Run the CI script tests locally:

```bash
pytest -q .github/scripts/tests/
```

Run the full set the same way CI does (also covers `tools/`):

```bash
pytest -q tools/tests .github/scripts/tests
```
