# `.github/` — CI, Workflows, and Repo Metadata

> See [`AGENTS.md`](../AGENTS.md) for the canonical agent guide (build/test/lint commands, coding
> conventions). This doc covers CI layout: workflows, composite actions, Python helpers, and
> build-config.

Platform workflows (`linux.yml`, `macos.yml`, `windows.yml`, `android.yml`, `ios.yml`) share logic
via composite actions and reusable workflows. Python helpers in `scripts/` are invoked by both.

Set the repository variable `CODECOV_TEST_ANALYTICS=true` to opt into JUnit uploads from Linux
and custom-build test jobs. Uploads use Codecov OIDC authentication and remain informational;
JUnit artifacts and existing test reporters remain available with the variable unset.

Docker publishes builder-image CycloneDX SBOMs and vulnerability scans alongside its existing CPM
reports. Trusted upstream push jobs attach provenance and SBOM attestations to the published image
digest; pull requests cannot run those publishing jobs. Master pushes also submit the Ubuntu
builder's CPM SPDX snapshot to the dependency graph. CPM metadata is read inside the builder so
container-local dependency paths resolve to the correct repositories.

Docker's BuildKit cache uses `type=gha,version=2`, scoped by variant and target. On RunsOn,
`runs-on/action@v2` initializes [Magic Cache](https://runs-on.com/docs/performance/caching/docker/)
before Buildx to store layers in S3. Only non-PR jobs export caches. Fork PRs use GitHub-hosted
runners and the ordinary GHA cache backend, without access to the private S3 cache.

ClusterFuzzLite PR runs use the bundled seed corpus without querying historical GitHub artifacts
(`NO_CLUSTERFUZZ_DEPLOYMENT=true`). This also disables previous-build crash comparison: reproducible
crashes fail the PR regardless of whether they predate it. Crash files and SARIF diagnostics are
uploaded separately. The master-only continuous build still publishes fuzzer binaries.

## Contents

- [Layout](#layout)
- [Workflows](#workflows)
- [Composite Actions](#composite-actions)
- [Scripts](#scripts)
- [Managed Runner Images](#managed-runner-images)
- [Build Configuration](#build-configuration)
- [Dependency Management](#dependency-management)
- [CI Conventions](#ci-conventions)
- [Tests](#tests)

## Layout

```text
.github/
├── workflows/                 # Platform builds, reusable workflows, and repo automation
├── actions/                   # Composite actions and external-action policy (see actions/README.md)
├── scripts/                   # Python helpers invoked by workflows and actions
│   ├── templates/             # Jinja2 templates (build_results.md.j2)
│   └── tests/                 # pytest suite for scripts/ (see #tests)
├── runner-images/             # Packer definitions and managed-runner provisioning
├── runs-on.yml                # Repository-level RunsOn images and runner shapes
├── build-config.json          # Centralized version numbers and build settings
├── build-config.schema.json   # JSON Schema for build-config.json
├── dependabot.yml             # Dependabot config (GitHub Actions only)
└── renovate.json              # Renovate config (code, tooling, and dev-environment dependencies)
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
| `ci-scripts.yml` | Lints workflows, validates runner images, and runs the CI Python script tests (see [Tests](#tests)) |
| `analysis.yml` | Static analysis |
| `codeql.yml` | CodeQL security scanning |
| `pr-checks.yml` | PR validation checks |
| `release.yml` | Release automation |
| `docs.yml`, `doxygen.yml` | Documentation deployment |
| `cache-cleanup.yml`, `cache-cleanup-pr.yml`, `_cache-cleanup.yml` | Cache maintenance (reusable + scheduled + PR-triggered) |
| `crowdin.yml`, `lupdate.yml` | Translation workflows |
| `dependency-review.yml` | Dependency security review |
| `scorecard.yml` | OpenSSF Scorecard |
| `stale.yml` | Nightly stale-issue labeling and closing (feature requests get their own close message) |
| `flatpak.yml` | Flatpak builds |
| `mirror-gstreamer.yml` | Mirror upstream GStreamer releases to the QGC S3 bucket |
| `px4-metadata.yml` | PX4 metadata sync |
| `runner-images.yml` | Manually build QGC's managed RunsOn AMI |
| `vm-builds.yml` | VM-based builds |
| `welcome.yml` | New contributor welcome |

## Managed Runner Images

The manual `runner-images.yml` workflow builds a QGC-specific Ubuntu 24 x64 AMI from the current
RunsOn base image and then boots it for a focused smoke test. It only publishes from the default
branch. The workflow uses the same dependency and Qt setup helpers as ordinary CI, and the
`qt-install` action reuses the preinstalled SDK only when its Qt version, architecture, and module
manifest satisfy the job's request. `ci-scripts.yml` also runs Packer formatting and validation on
pull requests without requiring AWS credentials.

The managed image and Windows warm-pool routes are opt-in repository settings. Without
`RUNS_ON_LINUX_BUILDER` or `RUNS_ON_WINDOWS_POOL`, every affected workflow keeps its existing
standard RunsOn runner. See the [runner image guide](runner-images/README.md) for AWS prerequisites,
activation, rebuild cadence, and the organization-level warm-pool example.

### TestFlight releases

`ios.yml` builds a Release device bundle and a Debug x86_64 simulator bundle. Pull-request and
branch builds remain unsigned. A `v*` tag selects the Xcode App Store preset, imports an Apple
Distribution certificate and provisioning profile, verifies the signed bundle, packages an IPA,
and uploads it to TestFlight.

Configure these repository variables before publishing a tag:

- `APPSTORE_BUNDLE_ID` (defaults to `org.mavlink.qgroundcontrol`)
- `APPSTORE_TEAM_ID`
- `APPSTORE_ISSUER_ID`
- `APPSTORE_API_KEY_ID`
- `APPSTORE_PROVISIONING_PROFILE_NAME`

Configure these repository secrets:

- `APPSTORE_API_PRIVATE_KEY` — App Store Connect API private key in PKCS#8 `.p8` format
- `APPSTORE_CERTIFICATES_FILE_BASE64` — base64-encoded Apple Distribution `.p12`
- `APPSTORE_CERTIFICATES_PASSWORD` — password for the distribution `.p12`

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
| `qt-install` | Reuse a compatible managed-image Qt SDK or install Qt via aqtinstall with caching |
| `qt-android`, `qt-ios` | Mobile Qt setup |
| `setup-python` | Python + uv + dependency installation |
| `size-analysis` | Binary size tracking (bloaty) |
| `test-duration-report` | Analyze JUnit test durations and summarize slow tests |
| `test-report` | Publish and upload test results |
| `verify-executable` | Post-build executable boot-test verification |

See [`actions/README.md`](actions/README.md) for the repository's external-action reference policy,
including how to handle automated warnings about major-version tags.

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
| `ios_boot_test.py` | Run the QGC smoke test in a disposable iOS simulator |
| `mirror_gstreamer.py` | Mirror official upstream GStreamer release artifacts to the QGC S3 bucket |
| `mold_helper.py` | Download and install a pinned, SHA256-verified `mold` linker binary (Linux) |
| `plan_docker_builds.py` | Generate Docker workflow build matrices from changed files |
| `precommit_results.py` | Normalize pre-commit outputs into uploaded CI artifacts |
| `release_builds.py` | Dispatch and freeze exact release workflow run identities |
| `report_context.py` | Reject stale PR/default-branch reporting contexts |
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

- **Dependabot** (`.github/dependabot.yml`) owns action references in `.github/workflows`, grouped
  weekly. Merge with `@dependabot merge`.
- **Renovate** (`.github/renovate.json`) owns `npm`, Python (pep621/uv), pre-commit, devcontainer,
  Dockerfile, Gradle Wrapper, and composite-action dependencies. Workflow paths are excluded so
  the bots do not open overlapping action updates.

## CI Conventions

### Cache lifecycle

CPM saves and uv/Python cache enablement use environment values that remain available
during nested composite post-job cleanup; step outputs do not survive that phase.
Compiler and moc caches use rolling write keys, with a distinct suffix per matrix leg.
macOS explicitly passes the installed ccache binary to CMake so Homebrew discovery
cannot select another version.

Scheduled cache cleanup protects the latest default-branch generation of each build
and dependency cache family, including the latest published `build-baseline-v2`
snapshot. PR and other branch caches remain evictable under storage pressure. Deletions
are scoped to the selected ref; failed deletions and an unreachable size target are
reported. Cleanup affects GitHub storage, not the RunsOn S3 backend.

Lychee restores and saves `.lycheecache` with a three-day entry lifetime; fork PRs only
restore. Android emulator tests create a fresh AVD; no AVD snapshots are cached.
Gradle, Flatpak, iOS target Qt SDK, and GitHub-hosted uv/Python caching remain disabled.

### Build helpers

- **CMake entrypoint**: Platform workflows configure through `cmake-configure`, which requires
  `qt-cmake` by default. Android is the explicit exception and supplies its target Qt toolchain and
  prefix to plain CMake.
- **Windows Qt architecture**: aqt architecture names are explicit package identifiers; CI tests
  require every workflow matrix to use the same MSVC generation when that identifier changes.
- **Dependencies**: CI Python scripts use `httpx` for GitHub API access and `jinja2` for
  templating. Deps managed in `tools/pyproject.toml` under `[dependency-groups] scripts`.
- **Shared helpers**: `gh_actions.py` provides GitHub API pagination (httpx) with `gh` CLI
  fallback. Import as `from common.gh_actions import ...`.
- **Bootstrap scripts** (`install_dependencies_helper.py`, `ccache_helper.py`): use stdlib only —
  they run before dependencies are installed.
- **Outputs**: use `common.gh_actions.write_github_output()` for `$GITHUB_OUTPUT` writes.

## Tests

`ci-scripts.yml` runs workflow linting, credential-free Packer validation, and pytest jobs on
changes under `.github/` and `tools/`. The pytest jobs cover both `tools/tests` and
`.github/scripts/tests`.

Run the CI script tests locally:

```bash
uv run --project tools --group scripts --group test pytest -q .github/scripts/tests
```

Run the full set locally with the same locked dependency groups CI installs (also covers `tools/`):

```bash
uv run --project tools --group scripts --group test pytest -q tools/tests .github/scripts/tests
```

## Validation tiers and build identity

- `pre-commit.yml` enforces fast hooks on changed files against the event's base SHA.
  Compiler-aware Clazy and clang-tidy hooks are manual locally; `analysis.yml` builds
  generated prerequisites and runs both for relevant PRs and weekly. Diagnostics are
  advisory, but missing tools, invalid databases, timeouts and compiler failures fail
  analysis. Full scheduled analysis prevents unchanged-code findings from disappearing.
- C++ CodeQL is built and uploaded only by Linux (`/language:c-cpp/build:linux`).
  `codeql.yml` handles Actions, Java/Kotlin and Python.
- `test-phase` shares Linux/custom unit and integration execution. Call it only after
  a successful build. Failures preserve JUnit, logs and durations and do not suppress
  the other suite. There are no blanket until-pass retries; empty test selections fail.
- `extended-tests.yml` runs a focused portable utility suite on native Windows and
  macOS Debug builds. Weekly/manual Linux runs exercise `Network|Flaky` tests normally
  excluded from PR jobs. iOS simulator builds run `--simple-boot-test` and require
  QGC's success marker. Windows installer verification lives in
  `deploy/windows/verify-installer.ps1`.
- Ordinary Docker PRs build Ubuntu 24.04 plus Android when affected. Toolchain,
  dependency and packaging changes, unknown diffs, pushes, dispatches and weekly
  schedules retain every configured variant.
- Reporting uses one workflow-run snapshot and treats every `completed` conclusion
  as terminal. Before posting or saving, it checks the current PR/master SHA.
  Baseline sizes, coverage and source run identities share an immutable commit/run
  cache key. Exact PR base snapshots are preferred before the most recent baseline.
- Each uploaded package includes `.build.json` producer identity, checksum and selected
  CMake configuration. Artifact API metadata retains IDs and available digests.
  Releases dispatch builds at the tag, poll exact run IDs and freeze their identities;
  artifact downloads reject changed run attempts or missing platform artifacts.
- `build-action` remains available. Platform workflows continue to compose the smaller
  setup/configure/build/test/package actions where they need distinct phases.

### Python environment contract

`setup-python` synchronizes frozen dependency groups from `tools/uv.lock` into
`tools/.venv` and exposes that environment's commands on PATH. Narrower setup calls
preserve installed groups. CMake uses the same interpreter and MAVLink generation
uses its preinstalled dependencies, without installing packages during a build.

CI Scripts runs the full Python suite on Python 3.10 and 3.12, with environment,
subprocess, network, Qt setup, and cache tests on Windows. A separate job checks
Ruff formatting, Pyright, and the `common` / `qgc_tools` / entrypoint import boundaries.
Local equivalents are `just test-python` and `just lint` after installing `dev`.

### Configuration and documentation checks

CI Scripts validates build schema relationships and release decisions using locked Node tooling.
Schema-only edits run validation. CodeQL C++ extraction stays in the Linux release build;
the standalone workflow scans Actions, Java/Kotlin, and Python, including deployment and tests.
Runner image candidates are smoke-tested by AMI ID before production promotion, with weekly
rebuilds and bounded retention; see [runner-images/README.md](runner-images/README.md).

Docs checks English internal links without exemptions before building all locales. Docs Lint
runs Markdown, spelling, and prose hooks for changed English pages without installing Qt.
External checks run weekly as well as on PRs; exact legacy URL exceptions expire on their
recorded review date. Translation files are maintained independently.

Code Analysis also accepts `qmllint` on manual dispatch. It builds generated modules first,
then enables missing-import/property/type errors against the SDK and build import directories.
