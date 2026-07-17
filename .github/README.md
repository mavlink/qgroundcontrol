# `.github/` — CI, Workflows, and Repo Metadata

> See [`AGENTS.md`](../AGENTS.md) for the canonical agent guide (build/test/lint commands, coding
> conventions). This doc covers CI layout: workflows, composite actions, Python helpers, and
> build-config.

Platform workflows (`linux.yml`, `macos.yml`, `windows.yml`, `android.yml`, `ios.yml`) share logic
via composite actions and reusable workflows. Python helpers in `scripts/` are invoked by both.

## Contents

- [Layout](#layout)
- [Workflows](#workflows)
- [Release Artifact Flow](#release-artifact-flow)
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
├── runner-images/             # Packer templates and RunsOn pool handoff configuration
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
| `clusterfuzzlite.yml` | AddressSanitizer fuzzing of the MAVLink byte-stream parser |
| `pr-checks.yml` | PR validation checks |
| `release.yml` | Release automation |
| `docs.yml`, `doxygen.yml` | Documentation deployment |
| `cache-cleanup.yml`, `cache-cleanup-pr.yml`, `_cache-cleanup.yml` | Cache maintenance (reusable + scheduled + PR-triggered) |
| `crowdin.yml`, `lupdate.yml` | Translation workflows |
| `dependency-review.yml` | Dependency security review |
| `scorecard.yml` | OpenSSF Scorecard |
| `flatpak.yml` | Flatpak builds |
| `mirror-gstreamer.yml` | Mirror upstream GStreamer releases to the QGC S3 bucket |
| `runner-images.yml` | Manually build the preloaded RunsOn Ubuntu image |
| `px4-metadata.yml` | PX4 metadata sync |
| `vm-builds.yml` | VM-based builds |
| `welcome.yml` | New contributor welcome |

### TestFlight releases

`ios.yml` builds both a Release device target and a Debug x86_64 simulator target. Simulator builds
run on the Intel macOS image and verify the resulting app bundle architecture. Pull-request and
branch builds remain unsigned. A `v*` tag switches the device build to the Xcode generator, signs
the app with an App Store distribution profile, verifies the resulting bundle, and uploads the IPA
to TestFlight with `apple-actions/upload-testflight-build@v5`.

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

## Release Artifact Flow

`release.yml` runs semantic-release, dispatches every configured platform workflow at the new tag,
waits for those builds, and then uses `download-all-artifacts` to collect their successful run
artifacts by head SHA. The release job uploads the desktop and Android binaries, their SHA-256
sidecars, and generated SBOMs with `gh release upload`. The iOS workflow participates in the release
gate and publishes its signed IPA to TestFlight; the IPA is not attached as a direct-download GitHub
release asset.

Linux AppImage and Android APK contents are extracted for release-time SBOM generation. macOS and
each Windows installer variant instead publish uniquely named SBOM artifacts from their native
build trees; the release job requires and attaches those artifacts rather than best-effort scanning
opaque DMG and EXE files on Linux.

Published builder images receive a CycloneDX image SBOM, a separate Grype code-scanning category,
and GitHub provenance and SBOM attestations. The existing CPM SBOM scan remains separate so source
dependencies that are not installed into the image stay visible.

Keep `.github/build-config.json`'s `build.platform_workflows`, `build-results.yml`'s
`workflow_run.workflows`, `release.yml`'s wait list, and `download-all-artifacts`' default in sync.
`test_workflow_policy.py` enforces that contract.

## Composite Actions

| Action | Purpose |
| --- | --- |
| `cmake-configure` | Configure QGroundControl from a repository CMake preset plus CI-only overrides |
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
| `coverage` | Generate and upload code coverage reports through Codecov OIDC |
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
| `test-report` | Publish test results and optionally upload them to Codecov Test Analytics through OIDC |
| `verify-executable` | Post-build executable boot-test verification |

## Scripts

Python entrypoints in `.github/scripts/` are invoked by workflows and composite actions. The
canonical inventory, shared-helper boundary, bootstrap rules, and local test command are documented
in [`.github/scripts/README.md`](scripts/README.md).

Reusable behavior belongs in `tools/common/`; workflow-facing entrypoints stay under
`.github/scripts/` so their paths remain stable. Every production script has a matching test, and
policy tests guard both the documented inventory and sparse-checkout dependency closure.

## Build Configuration

- **`build-config.json`**: Centralized version numbers (Qt, Android SDK/NDK, Apple/Xcode, CMake
  minimum, GStreamer) and build settings, validated against `build-config.schema.json`.
- Read values via `common.build_config.get_build_config_value()`, or via the `build-config`
  composite action from within a workflow step.
- **`CMakePresets.json`**: Canonical platform configure defaults. Every `cmake-configure` action
  call selects a preset; workflow `extra-args` are reserved for values that are inherently dynamic.

## Dependency Management

Dependency updates are split between two bots to avoid overlapping PRs:

- **Dependabot** (`.github/dependabot.yml`) owns `github-actions` updates only, grouped weekly.
  Merge with `@dependabot merge`.
- **Renovate** (`.github/renovate.json`) owns `npm`, `python` (pep621/uv), and `pre-commit`
  updates, grouped into a single weekly PR. GitHub Actions paths are excluded via `ignorePaths`.
- **Dependency submission** fills GitHub's manifest gaps. The primary Android build generates the
  Gradle dependency graph, and the canonical Linux Docker build generates an SPDX snapshot for CPM
  dependencies. Separate write-scoped jobs submit both snapshots after trusted `master` builds.

## CI Conventions

- **Job boundaries**: every executable job sets `timeout-minutes`, runs `harden-runner` before any
  checkout or third-party setup action, and disables checkout credential persistence. Jobs remain
  in audit mode until their observed endpoint lists are stable enough for block mode.
- **Permissions**: every workflow declares an explicit default permission boundary; write scopes
  belong on only the jobs that publish, attest, comment, or update repository state.
- **Dependency pins**: GitHub Actions use version tags according to repository policy, while
  workflow container images use immutable digests.
- **Dependencies**: CI Python scripts use `httpx` for GitHub API access and `jinja2` for
  templating. Deps managed in `tools/pyproject.toml` under `[project.optional-dependencies] scripts`.
- **Shared helpers**: `gh_actions.py` provides GitHub API pagination (httpx) with `gh` CLI
  fallback. Import as `from common.gh_actions import ...`.
- **Bootstrap scripts** (`install_dependencies_helper.py`, `ccache_helper.py`): use stdlib only —
  they run before dependencies are installed.
- **Outputs**: use `common.gh_actions.write_github_output()` for `$GITHUB_OUTPUT` writes.

### RunsOn performance

Docker jobs persist their mounted ccache directory separately from BuildKit layers. Trusted runs
outside pull requests write both caches; pull requests restore them without writing. On RunsOn,
BuildKit uses the runner's direct S3 cache variables and falls back to GitHub's cache backend
elsewhere.

The optional preloaded Ubuntu image and Windows warm-pool activation are documented in
[`runner-images/README.md`](runner-images/README.md). Both use repository variables so their
infrastructure can be deployed before active workflows select it.

## Tests

`ci-scripts.yml` runs two jobs on changes under `.github/` and `tools/`: `actionlint` (workflow
linting) and a pytest job covering both `tools/tests` and `.github/scripts/tests`. The latter
includes repository-wide workflow policy checks for permissions, timeouts, runner hardening, and
checkout credential persistence.

For the application test framework and CTest labels, see [test/README.md](../test/README.md). For
developer-facing `just` recipes, see [tools/README.md](../tools/README.md#quality).

Run the CI script tests locally:

```bash
pytest -q .github/scripts/tests/
```

Run the full set the same way CI does (also covers `tools/`):

```bash
pytest -q tools/tests .github/scripts/tests
```
