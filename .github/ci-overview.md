# `.github/` — CI, Workflows, and Repo Metadata

> See [`AGENTS.md`](../AGENTS.md) for the canonical agent guide (build/test/lint commands, coding
> conventions). This doc covers CI layout and conventions. Composite actions are indexed in
> [`actions/README.md`](actions/README.md) and CI Python helpers in
> [`tools/ci/README.md`](../tools/ci/README.md).

## Contents

- [Layout](#layout)
- [Workflows](#workflows)
- [Actions and Scripts](#actions-and-scripts)
- [Build Configuration](#build-configuration)
- [Dependency Management](#dependency-management)
- [Runners](#runners)
- [Change Detection and Release Gating](#change-detection-and-release-gating)
- [Caching](#caching)
- [Builds and Tests](#builds-and-tests)
- [Publishing and Reporting](#publishing-and-reporting)
- [Lint, Docs, and Security Checks](#lint-docs-and-security-checks)
- [Tests](#tests)

## Layout

```text
.github/
├── workflows/                 # Platform builds, reusable workflows, and repo automation
├── actions/                   # Composite actions and external-action policy (see actions/README.md)
├── runner-images/             # Packer definitions and managed-runner provisioning
├── runs-on.yml                # Repository-level RunsOn images and runner pools
├── build-config.json          # Centralized version numbers and build settings
├── build-config.schema.json   # JSON Schema for build-config.json
├── dependabot.yml             # Dependabot config (workflow actions only)
├── renovate.json              # Renovate config (code, tooling, and dev-environment dependencies)
└── zizmor.yml                 # Workflow security-lint policy
```

## Workflows

| Workflow | Purpose |
| --- | --- |
| `linux.yml`, `macos.yml`, `windows.yml` | Desktop builds, tests, and packages (`linux.yml` also builds the `custom-example` plugin) |
| `android.yml`, `ios.yml` | Mobile builds, emulator/simulator smoke tests, and store uploads |
| `docker.yml` | Builder images and containerized Linux/Android builds |
| `vm-builds.yml` | Builds inside the Multipass and Vagrant developer VMs |
| `flatpak.yml` | Manual Flatpak build |
| `_detect-changes.yml` | Reusable: decide whether a platform builds (`should_build`) and plan Android's and Docker's build legs (`matrix`) |
| `build-results.yml` | PR build-results comment, compared with the PR base's own builds (`workflow_run` trigger) |
| `release.yml` | Draft a GitHub Release with git-cliff notes for a pushed `vX.Y.Z` tag (assets are uploaded by hand) |
| `analysis.yml` | Code Analysis: clang-tidy and Clazy; manual IWYU, qmllint, and ThreadSanitizer |
| `codeql.yml` | CodeQL for Actions, Java/Kotlin, and Python |
| `clusterfuzzlite.yml` | MAVLink parser fuzzing on PRs |
| `pre-commit.yml` | pre-commit hooks (run by prek) on changed files, including docs prose and translation placeholders |
| `ci-scripts.yml` | Runs the CI Python tests on Linux and Windows (see [Tests](#tests)); lint and config validation run in `pre-commit.yml` |
| `check-links.yml` | Markdown link validation (lychee) |
| `pr-checks.yml` | PR title validation and size/file labels |
| `dependency-review.yml` | Dependency security review |
| `scorecard.yml` | OpenSSF Scorecard |
| `docs.yml`, `doxygen.yml` | User/developer guide and Doxygen API docs build and deployment (Doxygen runs weekly; pull requests only when its configuration changes; master runs keep their HTML artifact eight days for `pages.yml`) |
| `pages.yml` | The [GitHub Pages site](#github-pages-site): CI health dashboard of master builds, JSON schemas, master downloads, and the API docs |
| `crowdin.yml`, `lupdate.yml` | Crowdin sync and weekly source-string extraction |
| `px4-metadata.yml` | Daily PX4 airframe and parameter metadata update PR |
| `build-gstreamer.yml`, `mirror-gstreamer.yml` | Build GStreamer SDKs; mirror upstream GStreamer releases to the QGC S3 bucket |
| `build-profile.yml` | Manual CMake build profiling |
| `runner-images.yml` | Build, smoke-test, and promote QGC's managed RunsOn AMI (manual and weekly) |
| `cache-cleanup.yml` | GitHub Actions cache cleanup (scheduled, on PR close, and manual) |
| `stale.yml` | Stale-issue labeling and closing |
| `welcome.yml` | New contributor welcome |
| `copilot-setup-steps.yml` | Copilot cloud agent environment: Qt, system and Python dependencies, restored caches, and `just configure` (Copilot uses it only from the default branch; run it by hand to check the setup). Sessions (event `dynamic`) restore only the default branch's GitHub caches and cannot save, so they skip every save; daily and input-changing master runs save the apt download and Qt caches they restore, which master's RunsOn builds keep in S3 instead (sessions download CMake and Ninja). In sessions each slow step times out within Copilot's roughly 20-minute setup budget, so a stall fails that step and Copilot starts without the remaining steps rather than cancelling the session |

## Actions and Scripts

- Composite actions: one-line index, external-action reference policy, and behavior notes in
  [`actions/README.md`](actions/README.md). Platform jobs typically chain `build-setup`,
  `install-dependencies`, `cmake-configure`, `cmake-build`, tests, packaging, `verify-executable`,
  and `attest-and-upload`.
- Python helpers live in the `tools/ci/` package, grouped by concern (`planning`, `cache`, `compile`,
  `publish`, `reporting`, ...); workflows run them by path. Index, bootstrap rules, and the
  one-test-per-script rule (tests in `tools/tests/ci/`; workflow and action contracts in
  `tools/tests/ci/contracts/`) are in [`tools/ci/README.md`](../tools/ci/README.md). Reusable code
  lives in [`tools/common/`](../tools/common/README.md).
- Scripts reach the GitHub API through the `gh` CLI via `common.gh_actions` (`gh()` calls without
  an explicit `timeout` stop after 120 s; artifact downloads set their own) and write step outputs
  with `common.gh_actions.write_github_output()`. Third-party dependencies come from the `scripts`
  group in `tools/pyproject.toml`. Job summaries share one layout (see
  [Job summaries](#job-summaries)).
- Scripts that can run before `setup-python` use only the standard library and stdlib-only
  `tools/common`, `tools/qgc_tools`, and shared `tools/ci` library modules such as
  `reporting/workflow_runs.py`. Jobs without `setup-python` run `run_multipass.py`,
  `run_vagrant.py`, `read_config.py`, `update_px4_metadata.py`, and `tools/ci`'s
  `gh_cache_cleanup.py`, `mirror_gstreamer.py`, `analysis_matrix.py`, `plan_platform.py`,
  `deploy_docs.py`, `collect_build_status.py`, `download_artifacts.py`, `gh_pr_size_label.py`,
  `report_context.py`, `apt_install_retry.py`, `enable_kvm.py`, `install_multipass.py`, and
  `runner_image.py`. Steps ahead of a later or conditional `setup-python` also run
  `release_branch_check.py` (release tag check), `gstreamer/ci_plan.py` (Build GStreamer's
  plan), `gstreamer_archive.py` and `build_gstreamer.py` (Android/iOS GStreamer builds),
  `install_dependencies_helper.py` and `tools/setup/install_dependencies`
  (`install-dependencies`), `build-setup`'s `build_setup.py` planner,
  `setup-python`'s own `cache_backend.py` probe, plus the
  `install_python.py` and `install_uv.py` installers.
  `tools/tests/test_stdlib_only.py` imports each under `python -I -S`, and every module of the
  packages in `STDLIB_ONLY_PACKAGES`, and fails when a job without `setup-python` runs an unlisted
  script.
- CI, Docker, VM, sysroot, and packaging glue uses Python entrypoints; shell remains only for
  installing Python itself and loading container login profiles.
- In GitHub Actions, an uncaught exception in a CI Python entrypoint becomes an error annotation
  (`tools/_bootstrap.py`); for a failed command it names the command and quotes the last ten lines
  of its captured stderr (else stdout), with URL credentials and `Authorization` values hidden,
  and stdout escapes characters that the Windows code page cannot encode.
  `common.proc.run_with_retry` logs the same before raising its final failure. Set
  `RUNNER_DEBUG=1` (re-run with debug logging) or `QGC_CI_DEBUG=1` to add the fault handler,
  line-buffered output, and annotations for unraisable exceptions.
- Routine timings ("Configure completed in 12s", "Tests completed in 71s") are plain log lines,
  not notices: the job summary already reports them, and annotations are kept for problems.

## Build Configuration

- **`build-config.json`** centralizes versions and settings (Qt, Android, Apple/Xcode, CMake
  minimum and platform workflow list, GStreamer, analysis toolchain pins, and in `tools` the
  ccache, mold, sccache, Grype, Packer, RunsOn CLI, and ClangBuildAnalyzer releases with their
  SHA-256 digests and the Bloaty commit that `tools/ci` scripts and `build_profile.py` install),
  validated against `build-config.schema.json`
  (`just validate-configs`).
- **CMake and Ninja**: every CI build job runs the `build.cmake_version` and `build.ninja_version`
  that `build-setup` installs with `lukka/get-cmake`, after `setup-python` so they precede any CMake
  in `tools/.venv`; `install-dependencies` installs neither, nor the distro ccache and mold
  (`CI_PROVIDED_TOOLS` in `install_dependencies_helper.py`; `cache` installs pinned ccache and
  mold). Local development uses the `build` group's CMake, Ninja and just (`install_dependencies`
  syncs it on Linux). get-cmake resolves versions from the catalog bundled in its release, so the CMake pin cannot
  exceed the action's tag.
- Read values with `tools/setup/read_config.py --get <dotted.path>`,
  `common.build_config.get_build_config_value()`, or, as step outputs,
  `read_config.py --github-output <dotted.path>...` (dots become underscores). `build-setup`'s
  planner outputs the versions its steps need, and `qt-install` reads the Qt version itself, so
  its callers pass none.
- **Qt installer**: `qt-install` runs `install_qt.py`, which installs Qt with QGC's installer
  (`tools/setup/qt_installer`, standard library plus `cmake -E tar`); aqtinstall stays as the
  fallback through `QGC_QT_INSTALLER`. Every job reaching `qt-install` has CMake: `build-setup`
  installs it first, `pre-commit.yml` syncs the `dev` group, and `lupdate.yml` uses the
  GitHub-hosted image's ([tools/README.md](../tools/README.md#qt-installer)).

## Dependency Management

Dependency updates are split between two bots to avoid overlapping PRs:

- **Dependabot** (`.github/dependabot.yml`) owns action references in `.github/workflows`, grouped
  weekly. Merge with `@dependabot merge`.
- **Renovate** (`.github/renovate.json`) owns `npm`, Python (pep621/uv), pre-commit, devcontainer,
  Dockerfile, Node (`docs/.nvmrc`), Android Gradle, and composite-action dependencies, including Docker
  variant base images, the uv `required-version` floor, and the CI CMake (following get-cmake's
  tags) and Ninja pins in `build-config.json` via regex managers. Workflow paths are
  excluded so the bots do not open overlapping action updates. Updates arrive as weekly grouped PRs;
  major updates wait for approval on the Dependency Dashboard issue. `androidx.core` is not updated
  because newer releases require a compileSdk newer than Qt supports. A new release of a
  `build-config.json` `tools` pin also waits for approval there and then opens its own PR, which
  must add the release's SHA-256 digests before its checksum checks pass.

## Runners

### RunsOn runners

- Upstream jobs (`github.repository_owner == 'mavlink'`, which includes PRs from forks) run on
  RunsOn pools defined in `.github/runs-on.yml`; workflows in independent forks fall back to
  GitHub-hosted runners and GitHub's cache. macOS, iOS, Windows arm64, and Android's macOS leg
  always use GitHub-hosted runners.
- Every RunsOn job requests one label,
  `runs-on=<run_id>-<job id>-<run_attempt>-<matrix entry>/runner=<pool>`. GitHub hands a runner to
  any queued job with an identical label, so shared labels let matrix siblings steal each other's
  runners. The job id is written literally (`github.job` is empty outside steps) and the matrix
  entry is `strategy.job-index`, which matrix values cannot read, so every label is built in the
  job's `runs-on`; `linux.yml` matrix entries supply only the pool and GitHub-hosted fallback.
- Pools list exact instance types in on-demand priority order, and every event, pull requests
  included, uses on-demand capacity: a reclaimed Spot instance discards a whole build (three
  Windows PR builds were lost at once on 2026-10-23). The event type never changes a job's label.
- RunsOn reads `runs-on.yml` only from the default branch, so an unknown `runner=` fails to launch
  and labels repeat settings that must apply before a `runs-on.yml` change merges (for example the
  Windows builder's 130 GB volume). The `runs-on-lint` hook runs `roc lint` on the config when it
  changes, and `test_runs_on_config.py` keeps labels and pools in sync.
- `runs-on/action` enables Magic Cache and the cost summary (plus sccache's S3 settings in
  `linux.yml` Release legs when the repository variable `QGC_COMPILER_CACHE` is `sccache`; see the
  [sccache pilot](#sccache-pilot)), and skips itself off RunsOn, so
  jobs call it without a condition; RunsOn's built-in resource
  charts appear in each job's "Complete runner" step, so paid CloudWatch metrics stay off. The
  Harden-Runner community tier audits GitHub-hosted runners only, so jobs that can run on RunsOn
  skip it there.
- **apt mirror**: `apt_install_retry.py --update` points the Ubuntu archive (`archive.ubuntu.com`,
  or `ports.ubuntu.com` on arm64) at the region's EC2 mirror
  (`${RUNS_ON_AWS_REGION}.ec2.archive.ubuntu.com` or `.ec2.ports.ubuntu.com`), because the public
  archive sometimes served RunsOn jobs at about 200 kB/s or stalled `apt-get update` for minutes.
  Security entries stay on their original host, as on Canonical's EC2 images, and apt still
  verifies the signed release files over plain HTTP. A mirror that doesn't resolve is not used; if
  the mirror's first update fails, the original sources are restored and the normal retries run.
  Off RunsOn it does nothing; later `apt-get` calls in the job, the Multipass installer, and the
  Vagrant step reuse it. `--update` without packages only refreshes, and the apt download cache key
  does not depend on the mirror.

### Managed runner images

- `runner-images.yml` (manual, plus a weekly schedule once `RUNS_ON_AMI_SUBNET_ID` is set) builds
  a QGC Ubuntu 24 x64 AMI from the RunsOn base image, smoke-tests the candidate by AMI ID, and
  promotes it only from the default branch. It uses the same dependency and Qt setup helpers as
  CI, and `qt-install` reuses the preinstalled SDK only when its Qt version, architecture, and
  module manifest satisfy the job's request. The `packer-validate` hook validates the Packer template
  on pull requests that change it, without AWS credentials.
- The managed image (`RUNS_ON_LINUX_BUILDER`) and the organization-level Windows warm pool
  (`RUNS_ON_WINDOWS_POOL`) are opt-in repository variables; unset, workflows keep the standard
  RunsOn runners. See [runner-images/README.md](runner-images/README.md) for AWS prerequisites,
  activation, rebuild cadence, retention, and the warm-pool example.
- The Windows builder uses `windows25-full-x64`, which ships the MSVC toolchain
  (`windows22-full-x64` does not). The dependency installer skips Visual Studio Build Tools when an
  existing instance already provides every required component. The image takes about 75 GB, so the
  builder's volume is 130 GB, about 55 GiB free for the build (100 GB, 24.6 GiB free, ran out).

### Build timelines and runner debugging

- Linux (including its custom-plugin job), Windows, macOS, Android, iOS, and Docker jobs register
  `Kesin11/actions-timeline` before checkout, adding runner queueing and step timing to the job
  summary. Each timeline is a snapshot at that job's completion; matrix siblings may still be
  running, and nested composite steps stay collapsed.
- To inspect a failed runner, manually dispatch one of those workflows on a branch with
  `debug_runner` enabled. Each failed matrix job opens its own actor-only `mxschmitt/action-tmate`
  session after its normal reporting steps; connect with the SSH command in the step log and a key
  registered to the dispatching account, and run `touch continue` in the checkout to finish.
  Setup and session share a 15-minute step timeout, bounded by the remaining job timeout.
- Successful builds, PRs, tag runs, and cancelled jobs never open a session. A checkout failure can
  prevent the local action from loading; post-job cleanup failures and exhausted job timeouts
  cannot be inspected. Windows installs MSYS2 and tmate only when a session opens.

## Change Detection and Release Gating

### Change detection

- Platform workflows (Linux, macOS, Windows, iOS, Android, Docker) call `_detect-changes.yml`
  (job `changes / Detect Changes`): one sparse checkout and one `plan_platform.py --platform <name>`
  call that runs `detect_changes.detect_builds()` once and outputs `should_build` and `matrix`.
  For Android (`android_matrix.py`) and Docker (`plan_docker_builds.py`), `matrix` is the JSON
  include-list of build legs and `should_build` is whether it has any; other platforms get `[]`
  and define their own matrices.
- Pull requests build a platform only when its inputs changed (patterns in `detect_changes.py`);
  pushes, merge queues, and dispatches build everything; tags follow [Release tags](#release-tags).
- PR files come from `common.git.pr_diff_refs`, like the [lint checks](#lint-docs-and-security-checks):
  the detection checkouts fetch depth 2 and compare GitHub's merge commit with its first parent, so
  a PR that is behind its base branch does not build for the base branch's newer changes. Files are
  listed with `git diff --no-renames`:
  a moved file counts at both paths, so moving it out of a watched directory such as `src/` still
  triggers the builds that used it. If the diff can't be computed, builds run anyway.
- Docker PRs build Ubuntu 24.04 plus Android when affected (PRs touching only `android/` or
  `deploy/android/` run Docker Android). Toolchain, dependency, and packaging changes, unknown
  diffs, pushes, and dispatches build every variant. The full-matrix list in
  `plan_docker_builds.py` names Docker actions, scripts, dependency installers, and build inputs
  explicitly; unrelated GitHub metadata, analyzer scripts, tool tests, and VM provisioners do not
  expand the matrix.

### Required checks

- Linux, Windows, MacOS, Android, and iOS end with an `if: always()` gate job named
  after the workflow (`re-actors/alls-green`) that fails if change detection or any build leg
  fails or is cancelled; builds may be skipped only by change detection. Branch protection
  should require these five names, not build jobs: a skipped matrix job reports its unexpanded name
  (for example `Release ${{ matrix.arch }}`), and Android's legs differ by event.
  `tools/tests/ci/contracts/test_required_check_gates.py` pins the names and checks that each gate
  needs every other job in its workflow.
- These workflows run on every pull request and merge group. Path-filtered workflows (pre-commit,
  CI Scripts, Code Analysis, Docker, ...) can't be required: when a PR doesn't trigger them, their
  check never reports and the merge stays blocked.
- GitHub starts no workflows for pull requests opened with `GITHUB_TOKEN`, so `lupdate.yml` and
  `px4-metadata.yml` (upstream only) open theirs with the `PX4BUILDBOT_ACCESSTOKEN` secret, which
  also opens the Crowdin pull requests. Without it they fall back to `GITHUB_TOKEN`, and the
  required checks of those pull requests never report until someone pushes to the branch or closes
  and reopens the PR.

### Release tags

Platform workflows trigger on `push` of tags matching `v[0-9]+.[0-9]+.[0-9]+` only. Release tags
are cut on `Stable_V*` branches. Change detection (`_detect-changes.yml`, which also plans the
Android and Docker legs) skips every build for a tag ref that is not an exact `vX.Y.Z` name reachable from an
`origin/Stable*` branch (the name check also covers `workflow_dispatch` on a tag, which bypasses
the `push.tags` glob), so nothing is built, signed, or pushed (TestFlight, Play Store, Docker Hub).
`aws_upload.py publish` independently refuses to publish such a tag to the S3 `latest/` folder or
invalidate CloudFront. Both use `qgc_tools.release_tag_check.check_release_tag()`, which fails the
job if git cannot answer. Version-marker tags on `master` (e.g. `v5.2.0-dev`, consumed by
`git describe` for daily-build version numbers) do not match the trigger glob and never start a
build. Release tags are created and pushed by hand; the push also runs `release.yml`, which
stops with an error unless an `origin/Stable*` branch contains the tag
(`tools/ci/publish/release_branch_check.py`), so no release is drafted for a tag that is never
built. It then drafts the tag's GitHub Release with git-cliff notes (`release_notes.py`,
`tools/configs/cliff.toml`; `just release-notes vX.Y.Z` prints them locally) and writes them to the
job summary. A tag that already has a release, draft or published, keeps it unchanged; a manual
dispatch with the `tag` input drafts an older tag with the dispatching branch's tooling. Release
assets are uploaded by hand.

### TestFlight releases

`ios.yml` builds a Release device bundle and a Debug x86_64 simulator bundle, installing Qt's separate
`ios_device` and `ios_simulator_x86_64` SDKs. The simulator stays on x86_64 because Qt's bundled FFmpeg
has no arm64 simulator slice. Pull-request and branch builds remain unsigned. A release tag selects the
Xcode App Store preset, imports an Apple Distribution certificate and provisioning profile, verifies the
signed bundle, packages an IPA, and uploads it to TestFlight. The job's `APPSTORE_RELEASE` requires every
variable and secret below except the bundle ID; a tag without them builds the unsigned IPA, skips
TestFlight with a notice, and does not fail (`ios_package.py package` verifies the signature only when
`APPSTORE_RELEASE` is `true`).

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

### Secrets on pull requests

Same-repository PRs, including Renovate's, receive repository secrets even though they run
PR-controlled code. Signing and registry secrets therefore resolve empty on `pull_request`
events: Android PR builds sign with a throwaway debug keystore, macOS PR bundles are never
signed or notarized, and PR Docker builds do not log in to Docker Hub. Publishing steps
(AWS uploads, TestFlight, Play Store, docs deploys) are gated to pushes, tags, or dispatch.

## Caching

### Backends and planning

- Two backends: GitHub's cache, and RunsOn's
  [S3 cache backend](https://runs-on.com/docs/performance/caching/actions/) for upstream RunsOn
  jobs. They are separate; matching keys and paths cannot bridge them.
- `setup-python` runs `tools/ci/cache/cache_backend.py` from a JavaScript action, where the runner
  injects the cache URLs cache actions actually use (shell steps do not see them), and exports
  `QGC_ACTIONS_CACHE_BACKEND`. S3 requires the RunsOn
  proxy and its original GitHub endpoint, so a configured bucket alone (or a Windows stack before
  RunsOn 3.1) keeps GitHub limits. S3 backends also enable uv/Python caching with
  `UV_PYTHON_PREFERENCE=only-managed`, so uv caches standalone CPython instead of the image's Python.
- The `cache` action runs `cache_plan.py` once to resolve the save policy, cache scope, every key,
  and the compiler/moc/CPM environment. Default-branch pushes, schedules, and manual builds share the
  `shared` scope; PRs use `pr-<number>`. `pull_request_target` never saves under the default `auto`
  policy.
- Compiler, moc, CPM, and workspace Qt archives use workspace-relative paths (CPM sources live in
  `.cache/CPM`; CMake still receives an absolute `CPM_SOURCE_CACHE`), so keys match across hosted
  and RunsOn checkouts. There are no dedicated cache-warming builds, so a warm default-branch
  compiler cache is not guaranteed.
- Post-job saves (CPM, uv/Python) read environment values: nested composite step outputs are gone
  during post-job cleanup.

### Compiler and moc caches

- `cache` installs compiler tools in one step: `ccache_helper.py install` installs the pinned,
  SHA-256-verified ccache for the runner's OS and architecture (Linux under `/usr/local` plus mold;
  macOS and Windows under `RUNNER_TEMP`, added to `PATH`). Docker jobs skip it
  (`install-tools: false`).
- `save-build-cache` saves after a successful build or analysis autogen step, before packaging,
  tests, or scanners can fail; Docker saves when its report confirms successful compilation. One
  `ccache_helper.py save-plan` step runs the save gate below, packs the compiler/moc snapshot, and
  names the caches to save; the action uploads the snapshot or, when the upload fails or there is
  none, saves those caches.
- Pull requests on GitHub storage restore the default branch's compiler, moc, CPM, and GStreamer SDK
  caches but save none of their own (`scoped_save_policy` in `cache_plan.py`): those copies cost
  upload time and, for CPM, evicted shared entries from GitHub's size-limited cache. Merge-queue
  runs save none on either backend: their temporary ref is deleted after the run, so nothing could
  restore them. On S3, PRs (forks included) save to their PR scope, but
  skip a compiler or moc save when the build missed that cache fewer than 20 times
  (`PR_SAVE_MIN_MISSES` in `cache_plan.py`). Each save uploads a multi-GB archive (a PR build with
  99.7% hits re-uploaded 4.7 GB: 44 s on Linux, 159 s on Windows). The save gate in
  `ccache_helper.py save-plan` reads `ccache --print-stats` (Docker: the container's stats in
  `docker-build-report.json`) and the moc stats log, and saves if it can't read them or fails.
  Other events always save.
- On GitHub storage, compiler/moc snapshots are kept as one-day workflow artifacts
  (`build_cache_artifact.py`; see [`actions/README.md`](actions/README.md#action-notes)). Restores
  warn and fall back to the dependency cache on API errors, malformed responses, or timeouts (60 s
  per API attempt, 15 min for the download).
- GitHub compiler-cache budgets are 3 GB for Windows, coverage, and sanitizer builds, 2 GB for other
  Debug and macOS builds, and 1 GB for remaining Release builds; S3 backends use the `ccache.conf`
  ceiling, including PRs. Coverage's 3 GB budget is a trial: compare hits, LRU evictions, and
  transfer time on warm runs.
- Keys roll per run with a distinct suffix per matrix leg. moc save keys include the
  coverage/sanitizer variant while restore prefixes stay shared, so the shared moc cache holds every
  variant; the launcher's default `MOCCACHE_MAX_SIZE` is 1G (entries are mostly well-compressing
  include manifests). The job summary's Build section shows compiler and moc cache hit rates; a
  `miss-cache-evicted` row in its collapsed moc cache events means the limit is too small.
- Coverage keeps the compiler launcher (ccache stores `.gcno` files alongside objects). macOS passes
  the installed ccache binary to CMake so Homebrew discovery cannot select another version.
- Configure-check results (`check_include_file`, `check_symbol_exists`, and similar; about 650
  `try_compile` runs, mostly libarchive and SDL3) ride in the ccache archive under
  `qgc-configure-checks/`. `cmake/modules/CheckResultCache.cmake` seeds a fresh configure with the
  results saved for the same toolchain, flags, and QGC options, salted with the CPM dependency
  fingerprint and runner image (`QGC_CHECK_RESULT_CACHE_SALT`) plus, at configure time, the
  installed system packages and include paths (`system_header_digest` in `cmake_helper.py`:
  SDL3 and libarchive probe headers such as `libudev.h` unconditionally), and saves them after a
  successful configure; a local fresh configure dropped from 29 s to 7 s with byte-identical
  generated config headers. sccache pilot legs run the checks.

### sccache pilot

A trial, off by default: setting the repository variable `QGC_COMPILER_CACHE` to `sccache` moves
the two `linux.yml` Release legs (not coverage, sanitizers, or other workflows) from the ccache
archive to [sccache](https://github.com/mozilla/sccache) on RunsOn's S3 cache bucket. Deleting the
variable restores ccache.

- `runs-on/action` (`sccache: s3`) exports the bucket, region, and a per-repository, per-platform
  prefix (`cache/sccache/<repository id>/<os>-<arch>/v1`) that the stack's cache lifecycle expires.
  `cache_plan.py` then selects sccache (`QGC_COMPILER_CACHE_TOOL`), skips the ccache archive restore
  and save, and configures the server (`SCCACHE_BASEDIRS`, `SCCACHE_IDLE_TIMEOUT=0`, an error log in
  `RUNNER_TEMP`); without the bucket, as on GitHub-hosted runners, it warns and keeps ccache.
  `ccache_helper.py install` adds the pinned, SHA-256-verified sccache and passes it to CMake as
  `QGC_CACHE_PROGRAM`. moccache keeps its own archive.
- Every branch shares the prefix, so pull requests only read it (`SCCACHE_S3_RW_MODE=READ_ONLY`);
  pushes, merge-queue, and manual runs write it. RunsOn's branch-scoped `remote_cache: sccache`
  (RunsOn 3.6.0 or later) would let pull requests write their own entries.
- CMake disables precompiled headers whenever sccache is the launcher: sccache keys a GCC PCH by its
  preprocessed text, which omits macro definitions, so an edited header can reuse a stale PCH
  ([mozilla/sccache#2562](https://github.com/mozilla/sccache/issues/2562), reproduced with sccache
  0.18.0 and GCC 13). Each compiled source that used `pch.h` then costs about 1.8 s more CPU with
  Release flags, which slows cold builds.
- With remote storage, sccache preprocesses every source before its lookup (direct mode needs a
  local cache), about 0.4 s of CPU per hit where a ccache direct hit takes milliseconds. That offsets
  the archive transfer it removes (a warm `master` build restored 4.4 GB in 22 s and saved it in 50 s).

To evaluate, set the variable and run `linux.yml` twice on the same commit with `workflow_dispatch`
(the second run measures warm hits). Compare each leg's Build step and job durations, and the job
summary's sccache statistics (hit rate, cache writes, errors, server error log), with recent ccache
runs. sccache reaches S3 directly, so the Magic Cache traffic line in RunsOn's "Complete runner"
step leaves it out.

### CPM and GStreamer SDK caches

- CPM uses immutable dependency-fingerprint keys (computed by `cpm_helper.py` for `cache_plan.py`): CMake files that
  declare a dependency (`CPMAddPackage`, `CPMDeclarePackage`, `CPMFindPackage`,
  `FetchContent_Declare`), option defaults, presets, `build-config.json`, and the CPM modules and
  patches, so ordinary build-script edits reuse the default branch's snapshot. Only Release jobs
  save; PR and branch Release jobs first try an exact restore of the default-branch snapshot and
  skip their scoped copy when it exists. Unchanged snapshots are not saved again. Keys follow host
  and target, so Debug legs (the iOS simulator, Linux's custom plugin) restore their platform's Release
  snapshot; a `cache-key-suffix` splits them (Android legs), and suffixed legs fall back to the
  unsuffixed platform's sources. A managed runner image's CPM seed fills an empty cache when its
  fingerprint matches.
- GStreamer SDKs have separate immutable caches, excluded from CPM archives. Only Windows, Android,
  and iOS jobs use them (desktop Linux and macOS use the system GStreamer), keyed on the `gstreamer`
  section of `build-config.json` (fingerprinted in `cache_plan.py`), `cmake/GStreamer/Download.cmake`,
  and `cmake/GStreamer/platform/*.cmake`. They hold the extracted SDK trees without the downloaded
  archives; configure uses a tree stamped with the pinned checksums as is.

### Other caches

- **Qt SDKs**, including iOS and Android target SDKs, are cached by host, target, architecture,
  version, and a digest of the modules, archive subset, and (except for aqt) installer; PRs from
  forks only restore.
- **apt**: Linux `install-dependencies` caches downloaded `.deb`s per distro, architecture, month,
  and package-list hash (`apt-debs-…`, planned by `install_dependencies_helper.py plan`); a
  `category` install uses its own `apt-debs-<category>-…` family, so it never restores into or
  replaces the full set's cache. It saves right after a successful install, except in Copilot
  sessions, which cannot write the cache.
- **Gradle**: Android jobs cache Gradle User Home through `setup-gradle`'s open-source `basic`
  provider, keyed on the Gradle build files, only on the S3 backend; on GitHub's cache it evicted
  compiler caches (#14474), so GitHub-hosted legs such as Android on macOS don't cache it. Saves
  follow the `auto` policy, so pull requests save too. The key hashes every `**/*.gradle*` file in
  the workspace, so `build-setup` runs it before restoring the CPM cache and installing Qt: Windows
  cannot stat `android_openssl`'s directory symlinks there, and those Gradle files aren't QGC's.
- **pre-commit** restores prek's hook environments (`prek-v1-*`: `~/.cache/prek` without its download
  and build caches) keyed on `.pre-commit-config.yaml`, falling back to an earlier configuration's
  environments. prek installs only the environments of hooks that run, so a push that misses the
  key first runs `pre_commit.py --prepare-cache`, which prepares every hook (`prek prepare-hooks`,
  except TruffleHog and the Renovate validator) and then prunes unused environments
  (`prek cache gc`, which only warns on failure), and saves; PRs only restore. Docs
  Lint and Copilot Setup Steps restore the same entry read-only.
- **lychee** restores and saves `.lycheecache` with a three-day entry lifetime; fork PRs only restore.
- **Docker BuildKit** layers use `type=gha,version=2`, scoped by variant and target. On RunsOn,
  `runs-on/action` initializes [Magic Cache](https://runs-on.com/docs/performance/caching/docker/)
  before Buildx to store layers in S3. Only non-PR jobs export layer caches.
- **Not cached**: Android emulator AVDs (each run creates a fresh AVD), Flatpak, and uv/Python on
  GitHub-hosted backends.

### Cleanup

`cache-cleanup.yml` (`gh_cache_cleanup.py`, built-in `gh cache` commands) runs every six hours,
when a PR closes, and on demand; the script reads each trigger's settings from the event:

- Scheduled cleanup removes superseded PR build-cache generations even below the storage threshold,
  keeping the newest run/attempt per PR and matrix cache family. Above the threshold it evicts least
  recently used archives first, so newly populated larger caches survive, while protecting the
  latest default-branch generation of each build and dependency cache family, the prek hook
  environments, and the lychee link cache.
- Closing a PR removes that PR's merge-ref caches, without a job summary.
- Manual runs default to a dry-run prune; `dry-run: false` deletes, and `mode: wipe` lists (or
  deletes) every cache of the selected branch, or of all branches.
- Deletions are scoped to the selected ref; failed deletions and an unreachable size target are
  reported. Cleanup affects GitHub storage, not the RunsOn S3 backend.

## Builds and Tests

### Compiler settings

- **Unity builds**: Linux coverage and sanitizer builds and the iOS simulator build inherit the
  hidden `lean-debug` preset (`-g1` plus `QGC_UNITY_BUILD=ON`); the macOS Debug leg passes
  `-DQGC_UNITY_BUILD=ON`. Unity batches halve compile CPU. Bundled third-party code, the GPS
  protocol and GStreamer libraries, and files marked `SKIP_UNITY_BUILD_INCLUSION` stay per-file;
  Release jobs stay per-file. A redefinition or ambiguity error means two `.cc` files in one batch
  share file-local names or a file-scope `using namespace`: rename them or mark the file, and
  reproduce with `-DQGC_UNITY_BUILD=ON`. Apple builds batch a different source list than Linux, and
  adding or removing a source shifts later batches, so such errors can appear in files a PR did not
  touch (and the shifted batches miss the compiler cache once).
- **Line-table debug info** (`-g1`) is all gcov and sanitizer stacks need: it cuts objects by ~70%
  and compile CPU by ~17% against full `-g`, and keeps the iOS simulator objects within the
  compiler-cache budget.
- **rcc launcher**: CMake routes `Qt6::rcc` through a launcher that pins the file timestamps rcc
  embeds, so fresh checkouts keep generated `qrc_*.cpp` objects cacheable (an exported
  `QT_RCC_SOURCE_DATE_OVERRIDE` or `SOURCE_DATE_EPOCH` still wins), and lowers rcc's default zstd
  level from 19 to 9 (about 10x faster for ~2% more resource data) when QtCore supports zstd.
- **CMake entrypoint**: `cmake-configure` requires `qt-cmake` by default. Android is the exception
  and supplies its target Qt toolchain and prefix to plain CMake. Every caller names a configure
  preset, which sets the build type and `QGC_BUILD_TESTING`. A failed configure annotates its first
  `CMake Error` and uploads `cmake-configure.log`, `CMakeCache.txt`, and
  `CMakeFiles/CMakeConfigureLog.yaml` (CMake 4 no longer writes `CMakeOutput.log` or
  `CMakeError.log`); a failed build annotates its first compiler, linker, or Gradle errors.
- **Android javac warnings**: `setup-java`'s `javac` problem matcher made each deprecation warning
  in the Gradle build an annotation; `android.yml` removes it after `build-setup`, and `cmake-build`
  annotates javac errors itself.
- **Windows Qt architecture**: aqt architecture names are explicit package identifiers;
  `test_cmake_presets.py` requires every workflow matrix to use the same MSVC generation.
- **Windows linker**: Debug dispatches of `windows.yml` pass `QGC_USE_LLD_LINK=ON` to link with
  `lld-link` when the runner has it. Release builds keep `link.exe`: lld-link cannot link the IPO
  (`/GL`) objects they use.

### Unit and integration tests

- `test-phase` runs the Unit and Integration suites for Linux Debug (coverage and sanitizers) and
  macOS Debug. Call it only after a successful build. Failures keep JUnit,
  logs, and each suite's job-summary section (`junit_report.py`: counts, failures with their first
  report lines, slowest tests; failed tests also become error annotations) and do not suppress the
  other suite. There are no blanket until-pass retries, and empty test selections fail. See
  [`actions/README.md`](actions/README.md#action-notes).
- Unit tests run one CTest process per CPU, including coverage (GCC merges `.gcda` counters under
  file locks). Integration tests use two processes by default and for coverage, where instrumented
  tests otherwise hit their timeouts; Linux sanitizers use one per CPU.
- Set the repository variable `CODECOV_TEST_ANALYTICS=true` to upload JUnit results from the Linux test
  jobs (including the custom plugin's) to Codecov test analytics (OIDC, informational). JUnit artifacts and job
  summary reports are produced either way.
- **Sanitizers**: `Linux-sanitizers` (ASan + UBSan, leak detection on) excludes the `NoSanitizer`
  label. Runtime options come from CTest: `cmake/QGCTest.cmake` sets `*SAN_OPTIONS`
  (`halt_on_error=1`) on every test, overriding the shell environment. ThreadSanitizer runs only on
  manual Code Analysis dispatch.
- **Coverage**: GCC uses atomic counter updates for worker threads; gcovr tolerates the known GCC
  negative-counter bug and reports discarded counters. Codecov receives only the Cobertura
  `coverage.xml`, over OIDC (fork PRs upload tokenless); the step summary, threshold check, and build-results comment read gcovr's
  `coverage-summary.json`. `coverage_report.py` fails before generating them when the tests wrote no
  `.gcda` counters. `verify_coverage_thresholds.py --build-dir` reads thresholds from
  `CMakeCache.txt` and compares exact covered/total counts, not gcovr's rounded percentages. Manual
  Linux coverage dispatches also run the normally excluded `Network|Flaky` tests serially against
  the same build after the coverage report; they have separate reports and still fail the job.
- Reproduce either Linux lane with `cmake --workflow --preset Linux-sanitizers` or
  `--preset Linux-coverage` (`just coverage` uses `Linux-coverage`).
- **Portable tests**: Windows x64 and the macOS Release leg build and run ten standalone portable
  utility executables (`QGC_BUILD_PORTABLE_TESTS=ON`, CTest label `Portable`). They share test
  bodies with the full harness and do not enable test hooks in the packaged application. There is
  no separate Extended Tests workflow.

### Platform notes

- **macOS**: the Debug leg builds for arm64 only and runs the Unit and Integration suites; only the
  universal (x86_64h + arm64) Release leg packages and uploads the DMG. The job's `MACOS_SIGN`
  gates signing and notarization (CMake option, certificate import, and secrets) to non-PR Release
  legs in `mavlink` repositories or forks that set `MACOS_SIGNING_IDENTITY`; other forks build an
  unsigned DMG instead of failing in `SignMacBundle.cmake`.
  `verify_executable.py --type dmg` mounts the image, verifies the bundle, and always detaches it.
  On a boot-test timeout it captures the hung process's stacks before killing it and prints them
  after the captured output, and its `lipo`/`hdiutil`/AppImage helper
  calls time out after 120 s. The boot test forces QGC's log to stderr (a Windows GUI app otherwise
  logs to the debugger) and enables the startup categories with `--logging`, so a slow, hung or
  crashed boot shows where it stopped; a passing boot folds that log into a collapsed group. A crash
  is named by its signal or NTSTATUS (`-11 SIGSEGV`, `0xC0000005 STATUS_ACCESS_VIOLATION`) and the
  step exits 1: Python 3.10 cannot exit with a status above 2^31 on Windows, and MSYS bash reported
  the result as 127. On Windows QGC's unhandled-exception filter prints the code, address, and
  module+RVA to stderr and, with `QGC_CRASH_DUMP_DIR` set, has a thread started at launch write a
  minidump there (not the crashed thread, which may hold the loader lock or have no stack left),
  renamed from `.part` only when complete; failure
  diagnostics are uploaded as described in [`actions/README.md`](actions/README.md#action-notes). On
  Windows a build tree boots with only the GStreamer plugins the package ships (configure records them
  as `QGC_GST_SHIPPED_PLUGINS`; `verify_executable.py` copies them to `boot-test-gstreamer-plugins/`):
  the whole SDK's ~270 plugins took ~40 s to scan and hung or crashed QGC on exit.
- **iOS**: simulator builds run `ios_boot_test.py`, which launches with `--simple-boot-test` and
  requires QGC's success marker. Cold boots have a 600-second deadline and at most two fresh-device
  attempts (`--boot-timeout` 1–900 s, `--boot-attempts` 1–2). Boot and installation failures, and a
  launch that times out before QGC prints anything, are retried on a fresh device; QGC failures are
  not. Install and launch each have a 600-second deadline (`--timeout`), and every attempt's
  simulator commands share a 1500-second budget (`--total-timeout`) that ends them before the step's
  30-minute timeout. A shutdown/delete failure
  after a passing test is reported as a warning and does not fail the job; after a failed attempt it
  prevents the retry. Each attempt keeps command output; a failed attempt in any phase saves the
  simulator state, the last 10 minutes of the simulator's and CoreSimulator's logs, and QGC's new
  crash reports beside the log before cleanup. A failure ends in an error annotation naming the
  phase and attempt; every run writes a Boot test summary section, and the `ios-boot-test` artifact
  is linked below it.
- **Windows**: installer verification lives in `deploy/windows/verify-installer.ps1`.
- **Android**: legs are Linux (primary), macOS, Windows (not on PRs), and a Linux emulator smoke
  test. PRs build `arm64-v8a` only; other events build `arm64-v8a` + `armeabi-v7a` on the primary
  leg. Android jobs install only the `core` apt category. The Linux leg runs Android Lint
  (`android_lint.py`, `gradlew lintRelease` in `android-build`): up to ten Error and Fatal issues
  from lint's XML report become error annotations titled with the issue id, at the `android/`
  source that androiddeployqt copied (`QGC_ANDROID_PACKAGE_SOURCE_DIR`; a copy that differs, like
  the rewritten `AndroidManifest.xml`, keeps the file but not the line), or Gradle's errors when
  lint wrote no report; an Android Lint job-summary section follows the Build section, and a
  failure uploads the HTML and XML reports and the Gradle log for seven days, linked below it.
  Master pushes also build a signed multi-ABI App Bundle on the primary leg through `cmake-build`
  (`target: aab`, logged to `qgc-aab-build.log` with its own "Build (aab)" section and failure
  annotations; it saves no build caches), uploaded as its own attested GitHub artifact
  (`QGroundControl-linux-<abis>-aab`) but not to AWS or the Play Store. Tags start a staged Play
  Store production rollout; master pushes complete a beta release.
- **Custom plugin** (`linux.yml`'s `custom-plugin` job, behind the Linux gate): the `custom-example`
  overlay changes the app name and feature set, global compile definitions, so it is its own build:
  one Debug build that runs the Unit suite (excluding `StockUI`) and boots the build tree; the stock
  Linux legs run the integration suites and cover Release compiles. Outside pull requests it also
  installs the build stripped (`cmake --install --strip`; the Debug binary carries ~800 MB of debug
  info) into an AppImage and boots that. `custom-example/` changes trigger the Linux workflow.
- **ClusterFuzzLite**: PR runs use the bundled seed corpus without querying historical artifacts
  (`NO_CLUSTERFUZZ_DEPLOYMENT=true`), which also disables previous-build crash comparison:
  reproducible crashes fail the PR whether or not they predate it. Crash files and SARIF
  diagnostics are uploaded separately. The workflow runs only on pull requests: nothing reads
  uploaded fuzzer builds, so there is no continuous build.

### Docker

- One multi-stage `deploy/docker/Dockerfile` builds every variant in `deploy/docker/variants.json`.
  Base images come from the ECR Public mirror of Docker's official images
  (`public.ecr.aws/docker/library/<image>:<tag>@<digest>`, same digests as Docker Hub), so PR builds,
  which don't log in to Docker Hub, avoid its anonymous pull limit. Buildx's `moby/buildkit` image
  and the `docker/dockerfile:1` frontend still come from Docker Hub. No QEMU: every target builds
  for the x86_64 runner, and aarch64 cross-compiles.
- Application builds use the shared `cache` action (separate from the BuildKit layer cache) with
  keys that separate image variants and compiler identities. The action skips host tool
  installation and resets/reports statistics inside the container. The workspace mount exposes
  `.cache/ccache`, `.cache/moccache`, and `.cache/CPM` at `/project/source`; only cache budgets and
  statistics settings are forwarded from the host environment.
- Each job uploads `docker-build-report.json` (configure/build/package timings, cache statistics,
  CPM source size) with its Ninja build profile. Checkout keeps full history for package
  versioning; measured checkouts take 13–22 s, which doesn't warrant a repository mirror.
- x86_64 Linux variants also produce a native package (`.deb`, `.rpm`, or `.pkg.tar.zst`).
  `docker_helper.py exec` runs repository scripts in the builder image from the read-only
  workspace mount, translating host paths to container paths; as root it runs
  `tools/ci/packaging/validate_native_package.py --install`, which validates, installs,
  smoke-tests, and always uninstalls the package. `attest_helper.py checksum` requires and
  verifies CPack's `.deb`/`.rpm` checksum sidecar and writes Arch's.
- Pushes and manual runs generate CycloneDX SBOMs for CPM dependencies and the builder image and
  scan them with Grype (SARIF to code scanning); PRs skip SBOMs and scans. CPM metadata is read
  inside the builder, with the build tree mounted where it was built, so container-local
  dependency paths resolve. Master pushes submit the Ubuntu builder's CPM SPDX snapshot to the
  dependency graph.
- The planner gives every leg the builder image's push target (`resolve_push_target()` in
  `qgc_tools/docker_variants.py`): upstream pushes publish master and `Stable*` builds to GHCR and
  release tags to Docker Hub; pull requests, forks, and manual runs push nothing. The build,
  registry logins, and the `attest-images` job, which attests provenance and SBOM for each
  published image digest, follow `matrix.push_target`. The `docker` action keeps the pushed
  image's registry name as its local tag (`image` output).

### VM builds

- `vm-builds.yml` builds QGC inside the Multipass and Vagrant (libvirt) developer VMs on the
  nested-virtualization `linux-x64-vm-builder` pool; both jobs open `/dev/kvm` with
  `enable_kvm.py`. It runs when the VM provisioning inputs change (`deploy/multipass`,
  `deploy/vagrant`, the `tools/setup` installers they call, `.github/build-config.json`, and the
  presets). `.github/runs-on.yml` triggers pushes only: RunsOn reads it from the default branch, so
  a pull request run would still use the old pool.
- **Multipass** runs the client as root: the job has no user session, so only root's `snap run` can
  place the client in the snap cgroup snap-confine requires. `multipass launch` waits up to 15
  minutes (`MP_TIMEOUT`) instead of the 5-minute default, which image download and first boot under
  nested virtualization can exceed. The confined client cannot access the workspace or `/tmp`, so
  the source archive and the built AppImage pass through a temporary directory under root's home.
  `install_multipass.py --allow-bridge-forwarding` resets the iptables `FORWARD` policy that Docker
  sets to `DROP`, which would leave the guest without network (libvirt adds its own rules for
  Vagrant). Cleanup only deletes instances it created. The job stays `continue-on-error`.
- **Vagrant** runs `deploy/vagrant/run_vagrant.py` (`install`, then `build`): `vagrant up` stops
  after 150 minutes (`QGC_VM_UP_TIMEOUT`), the VM is destroyed only after an attempted startup, and a
  failed destroy fails the job; Vagrant failures fail the workflow. Reproduce locally with
  `python3 deploy/vagrant/run_vagrant.py install`, then
  `OUTPUT_DIR=artifacts python3 deploy/vagrant/run_vagrant.py`.
- Vagrant provisioning uses `deploy/vagrant/provision.py` and shares
  `deploy/multipass/build_in_vm.py` with Multipass. To rebuild inside a provisioned guest, run
  `BUILD_DIR="$HOME/shadow_build" python3 /vagrant/deploy/multipass/build_in_vm.py --skip-dependencies`
  as the `vagrant` user. Qt versions and modules come from `.github/build-config.json`.

### Code Analysis

- `analysis.yml` runs on pull requests and `master` pushes over one path list (the
  `&analysis-paths` anchor: compiled `src/` files plus `analysis_matrix.FULL_SCAN_INPUTS`); there is
  no schedule. Manual dispatch runs one job for the selected tool (`clazy`, `clang-tidy`, `iwyu`,
  `qmllint`, or `tsan`).
- `analysis_matrix.py` emits each job's `family`, packages, CMake configure arguments, and
  `tools/analyze.py` arguments, so one Configure step serves every family. For a
  PR it estimates the compilation units from the diff and a textual reverse-include scan of `src/`
  and allots one shard per 50 units, up to four; it plans none when nothing compiled changed, and
  full-scan inputs keep all four. Each shard runs clang-tidy then Clazy over one selection and one
  `clang-scan-deps` scan, and reports errors independently; only the first shard saves the shared
  build caches. Upstream jobs use the 16-vCPU `linux-x64-analyzer` pool; forks use GitHub-hosted
  runners.
- Each `master` push runs one job over every unit and saves `.cache/analysis-results`; every other
  run restores it read-only. Keys cover the tool binary and version, arguments and config, compile
  commands without output flags, and every scanned dependency's contents; tool or compiler errors,
  timeouts, and crashes are never stored; entries unused for 14 days or beyond 256 MiB are pruned.
  To force a full recompute, bump the `analysis-results-v1` cache key.
- clang-tidy comes from the PyPI wheel pinned by version and SHA-256 in `analysis.clang_tidy_*`
  (`build-config.json`); `tools/ci/runner/analysis_tools.py` reads those pins, installs it, verifies
  `.clang-tidy`, and puts it ahead of LLVM's own on `PATH`. Clazy and `clang-scan-deps` stay on
  `llvm_version`; Clazy is built from the pinned source revision by `tools/setup/build_clazy.py` and
  cached, and LLVM `-dev` packages install only to rebuild it.
- Compiler analysis uses Ninja, disables PCH and autogen's inherited link dependencies, and builds
  only `qgc-analysis-headers`; it does not compile or link QGC (`autogen` runs only for IWYU).
  qmllint and ThreadSanitizer keep full builds; ASan and UBSan run in `linux.yml`.
- Source-only PRs scan changed compilation units active in the build. Header changes use a fresh
  `clang-scan-deps` scan to include transitive dependents, falling back to all active project
  sources if the scanner is unavailable or incomplete; analysis/build configuration changes scan
  every active unit.
- Warnings are advisory; error diagnostics, missing tools, invalid databases, timeouts, and
  compiler-driver failures fail the job (`shell: bash` adds pipefail, so the log's `tee` keeps
  `analyze.py`'s status; `test_workflow_shells.py` requires it for every `| tee` step). Manual
  IWYU runs are advisory (`continue-on-error`); their uploaded report is the result.
  Each tool's job-summary section counts the unique errors and lists the first 20 ahead of its
  diagnostics listing (cut at 300 lines), and the first 10 errors are annotated on their lines.
  `misc-include-cleaner` is an advisory clang-tidy warning; it leaves
  Qt, MAVLink, GStreamer/GLib, and Qt-generated includes alone (`IgnoreHeaders` in `.clang-tidy`),
  because it would suggest their internal headers. Jobs upload `*-timings.json`,
  `*-progress.jsonl`, `*-diagnostics.json` (each unique diagnostic's file, line, column,
  severity, and check, and on pull requests those on changed lines), and `*-raw.txt` (every unit's full output; identical diagnostic blocks are
  shown once in the log).
  Per-check profiling (`profile_checks` dispatch input) is described in
  [`tools/analyzers/README.md`](../tools/analyzers/README.md).

## Publishing and Reporting

### Attestation and upload

- `attest-and-upload` runs `attest_helper.py prepare` (verifies or writes the artifact's `.sha256`
  sidecar and writes the CPM dependency SBOM and a `.build.json` manifest with producer identity,
  checksum, and selected CMake configuration), scans an SPDX SBOM, attests provenance and SBOM with
  `actions/attest` (separate invocations), and uploads the GitHub artifact.
- `prepare` also outputs `publish-aws`: true on pushes and tag dispatches in `mavlink` repositories
  when the caller keeps `upload-aws` and passes credentials (the action passes only whether it
  has them). The action then configures AWS credentials once and runs one `aws_upload.py publish`:
  artifacts go to `builds/<ref>/` (the runner's ref), and release tags also go to `latest/` with
  one CloudFront invalidation. Credentials come only from the environment.

### GitHub Pages site

`pages.yml` builds and deploys <https://mavlink.github.io/qgroundcontrol/> with
`tools/ci/publish/pages_site.py build --out _site`. Each deploy replaces the whole site, so every
build regenerates all four parts:

- **CI health dashboard** (`index.html`): each workflow's latest 60 master push runs as inline SVG
  charts (no scripts or external assets), every chart followed by a table of its latest values:
  wall time and Build-step durations (jobs API), distributable artifact sizes, JUnit test counts,
  failures and time per suite, `src/` line coverage (`coverage-summary.json`, as in the
  build-results comment), and Code Analysis's unique warnings per tool (`*-diagnostics.json`), plus
  the tests that failed on master in the last 30 days with their failed and total runs. Each
  chart column and table row links its run.
- **History**: `data/history.json` is versioned JSON with one record per (commit, workflow), read
  back from the live site on every build, so there is no data branch and the workflow never writes
  to the repository. Each build collects the last 10 completed master push runs of every platform
  workflow and Code Analysis that it lacks (or holds an earlier attempt of), newest first; records
  are dropped only past 1,000 commits or 365 days. Collection stops after 15 minutes and gives up
  on an artifact after two stalled 120-second attempts, so a slow artifact store skips runs, which
  the next build collects while they remain among the latest 10, instead of cancelling the
  30-minute build job; the site still deploys with every stored record. A history fetch error
  fails the build rather than deploy a truncated history. So does HTTP 404 once a `pages.yml` run
  has succeeded, since every successful run deployed the site and a Pages outage or unpublish
  also answers 404; only before the first deploy does the history start empty and backfill the
  last 30 runs per workflow whose artifacts remain. To start over after losing the deployed
  history, dispatch `pages.yml` with `new_history`. Artifacts are read from their zip archives,
  extracting only the reports needed.
- **Schemas** (`schemas/`): `src/FactSystem/factmetadata.schema.json`, whose `$id` must be its
  URL there (the build and `test_pages_site.py` check it), with an index page.
- **Downloads** (`downloads/`): the packages in `qgc_tools/build_artifacts.py`'s
  `PUBLISHED_BUILDS` under S3 `builds/master/` (`test_published_builds.py` keeps the list in
  sync with the workflows' `attest-and-upload` steps and the user guide), with the SHA-256 from
  each `.sha256` sidecar ("—" when missing) and the commit and run named by the package's
  provenance attestation, looked up by digest, plus `gh attestation verify` instructions.
- **API docs** (`api/`): the `doxygen_build` artifact of the latest successful non-PR Doxygen run
  on master, else (or when its download stalls past two 150-second attempts) a page linking
  <https://api.qgroundcontrol.com>. The Doxygen workflow still deploys that domain; moving it to
  the Pages site is a later admin step.

It runs on `workflow_run` completion of the platform workflows, Code Analysis, and Doxygen on
`master`, on master pushes that change the schema or the site tooling, daily, and on dispatch;
never on pull requests, whose artifacts must not reach the site (CI Scripts tests the tooling).
The build job runs only in `mavlink/qgroundcontrol` and, for `workflow_run`, only for a master
push run of this repository (Doxygen's scheduled or manual runs for Doxygen); it checks out the
default branch's tooling with `contents: read` and `actions: read`. The deploy job alone has
`pages: write` and `id-token: write`, in the `github-pages` environment. One `pages` concurrency
group serializes deploys without cancelling them. Pages must be enabled with source "GitHub
Actions" (Settings → Pages) before the first deploy.

### Build results

- `build-results.yml` runs on `workflow_run` completion of every workflow with a `pull_request`
  trigger: the platform workflows, pre-commit, Code Analysis, and the other checks (GitHub runs
  only the default-branch copy; `test_platform_workflow_names.py` keeps the list in sync with the
  workflow files and `build.platform_workflows`), and only for pull request builds. One
  `report_context.py` lookup resolves the PR number and the base commit the comparisons use:
  the base branch's tip when the triggering run was created
  (`commits?sha=<base ref>&until=<created_at>`), approximating the commit GitHub merged the PR
  head into for the build (a re-run builds the same merge and keeps `created_at`).
  `pull_request.base.sha` can be weeks older, since pushes to the PR need not update it, and
  would credit every later base-branch commit to the PR; it is used only when the lookup fails or
  finds nothing, which the step logs.
- The reporting scripts take the repository, commit, and event from the `workflow_run` payload
  (`workflow_runs.triggering_run()`), the platform workflows from `build.platform_workflows` in the
  sparse-checked-out `build-config.json` (failing when it is missing), and the run snapshot that
  `collect_build_status.py` saves in `$RUNNER_TEMP/workflow-runs-cache.json`; the comment
  generator reads each input from the path the earlier steps write (`INPUT_PATHS`). Steps pass only
  their own flags, and `test_build_results_workflow.py` replays them under fake payloads.
- Every trigger updates one comment (fork PRs included), so finished workflows and their failures
  show while others still run; the last one to complete renders the final state.
  `collect_build_status.py` (stdlib only, before Python dependencies install) writes each
  workflow's state, latest attempt's duration, attempt number, and wall time since the run was
  created, and its jobs with failed-step log links, from one jobs-API call per run
  (`--status-out`). A failed job whose runner was lost (a step left unfinished, or GitHub's "lost
  communication with the server" annotation, read only for a failed job without a failed step)
  names that step and its start and end. Every other workflow of the commit (CI Scripts, Docker,
  CodeQL, ...) is listed with its state, and its jobs only when it failed; each one's completion
  also triggers the comment, so a late failure still reaches the verdict.
- `download_artifacts.py` (one directory per artifact, plus a manifest naming its workflow and
  whether it downloaded) fetches the completed runs' test results, coverage, and distributable
  artifact listings (size, id, expiry), the pre-commit results, and Code Analysis's per-tool
  `*-timings.json` and `*-diagnostics.json`; runs still in progress are skipped. The same script
  then fetches the base commit's own `push` builds (`--head-sha "$BASE_SHA" --event push` with an
  empty `--runs-cache`): their test results, coverage, and artifact listings are what the comment
  compares with, each platform on its own, so a failed or missing base build only blanks its rows.
  Every download step continues on error, and the comment names artifacts that failed to download
  apart from those past retention, which the manifest marks `expired` and the comment notes.
  The comment is also the job summary.
- The comment (`generate_build_results_comment.py`) leads with a verdict (❌ failed, naming every
  failed workflow including other checks; ⏳ in progress; 🚫 cancelled; ⏭️ nothing to build; ✅
  passed), the linked head commit, how many platform workflows are done, pending, or skipped by
  change detection (every build job skipped), and how many other checks still run. A table gives
  each workflow's status, duration (with the attempt and total wall time after a re-run), test or
  hook counts, size change from the base, and run link; Code Analysis shows its warnings on lines
  the pull request changed. Lines follow for the other checks (problems linked first) and for the
  base commit with each platform's base run. A Failures section lists each failed job with links
  to its failing steps' logs (or the step its runner was lost during), failed tests with their
  first report lines (QtTest `FAIL!` blocks, sanitizer reports, else the output tail) from the
  JUnit XML, failed pre-commit hooks, and failed other checks. Collapsed sections cover per-suite
  test results with the tests each suite added or removed versus the base build; `src/` line,
  function, and branch coverage with changes in percentage points (test code is left out, as on
  Codecov, which the section links for the changed lines' coverage); artifact downloads with their
  size change (B, KB, or MB; ⚠️ above 1% and 1 MiB growth) and expiry; pre-commit counts when a
  hook failed; and Code Analysis's warnings on changed lines (`changed` in each
  `*-diagnostics.json`), linked at the commit the analysis checked out (`commit`, GitHub's merge
  commit on a pull request, whose line numbers can differ from the head's; the head commit for
  reports without one), with every unique warning counted only in a per-tool table. Lists are
  truncated with "…and N more", and longer reports shrink to stay under GitHub's comment limit.
  Names from pull requests (tests, jobs, hooks, artifacts) are escaped or shown as code. Android
  artifact names include their ABIs, so single-ABI PR APKs are not compared with multi-ABI
  release APKs; unmatched variants show `—` for their change.
- `report_context.py --comment comment.md` rechecks that the PR head is still the build's commit,
  then creates or updates the PR's comment marked `<!-- build-results -->` (or the marker of the
  comments `thollander/actions-comment-pull-request` posted before) through `gh api`.

### Job summaries

Scripts build each job-summary section with `common.markdown.summary_section`: an H3 title, an
`Item | Value` facts table, an optional body, and collapsed `<details>` for long listings, with
numbers, durations, and sizes from `common.format`. A build job reads top-down:

| Section | Written by |
| --- | --- |
| Setup | `build-setup` (`setup_summary.py`, also after a failed setup step, which it names): runner, image (RunsOn image and AMI, or the hosted `ImageOS` and `ImageVersion`), RunsOn instance type, lifecycle, zone and ID, CPUs and RAM, Qt version, installer and source, CMake and Ninja, compiler, Xcode, JDK, Android NDK, Python groups, cache backend, and each build cache's restore result with ccache's primary key, and the key a fallback (restore-keys) hit came from |
| Configure | `cmake-configure` (`cmake_helper.py configure`, also after failures): result, duration, preset, generator, configure-check reuse; after a failure the first CMake error and the end of the output collapsed, and a link to the configure logs |
| Build | `cmake-build` (`cmake_helper.py build`, also after failures): result, duration, toolchain, this build's compiler (ccache or sccache) and moc cache hit rates; failed targets and first errors, slowest steps, time by kind, and moc cache events collapsed; a link to the Ninja reports |
| Android Lint | `android_lint.py` on Android's Linux leg (also after failures): result, duration, error and warning counts; the first errors collapsed; after a failure a link to the lint reports |
| Unit Tests / Integration Tests (*suite*) | `run-unit-tests` (`junit_report.py`): counts, failures with their first lines, slowest tests and failure output collapsed; a link to the results artifact |
| Coverage, Binary size | `coverage_report.py --step-summary`, `size_analysis.py` |
| Boot test, Artifact | `verify_executable.py` (result with the crash's signal or NTSTATUS name, package, size, duration; after a failure the last log line, the last 40 lines collapsed, and a link to the diagnostics artifact) or, for the iOS simulator, `ios_boot_test.py` (result with the failing phase and attempt, simulator, attempts, duration, and the end of the boot log); and `attest_helper.py prepare` (file, size, SHA-256) |
| Disk | `disk-watch`'s post step, at the end of every `build-setup` and Docker job: the workspace volume's size and its start, peak (with free space and time), and end use, sampled every 10 s; a peak whose free space is below 10% of the volume, kept between 5 and 10 GiB, gets ⚠️ and a "Disk nearly full" warning annotation |

Docker builds write one Docker build section (`docker_helper.py summary`); pre-commit,
Code Analysis (`analyze.py --step-summary`), cache cleanup, GStreamer, release-notes, and Build
Profile jobs use the same layout. RunsOn's cost table and the build timeline come from their own
actions after these sections.

## Lint, Docs, and Security Checks

- Pull request diffs in pre-commit and Code Analysis come from
  `common.git.pr_diff_refs`: GitHub's merge checkout is compared with its first parent, the base tip
  it merged into (`pull_request.base.sha` can be older and would add newer base-branch changes), so
  these checkouts fetch depth 2. pre-commit pushes fetch full history to
  diff against `before`; a base with no merge base in the fetched history checks everything.
- `pre-commit.yml` runs on every path and `tools/pre_commit.py --ci --changed` (prek) on the pull
  request's or push's changed files, and uploads `pre-commit-results.json` and `pre-commit-output.txt` for the
  build-results comment. The C++ formatting hook checks modified regions against the PR merge base
  (or `HEAD` locally), following `CODING_STYLE.md`; new files and unchanged files supplied in a full
  sweep are checked in full. Compiler-aware Clazy and clang-tidy hooks are manual locally; CI runs
  them in `analysis.yml`. TruffleHog runs only as a local commit hook (`--ci` defaults `SKIP` to `trufflehog`). Qt is installed only when the selection includes QML for qmllint
  (`pre_commit.py --needs-qt`).
  `check-renovate` schema-checks `.github/renovate.json` in every run. `pre_commit.py --ci` also
  runs the manual-stage validators that download pinned tools, each only when its own files change:
  Renovate's `renovate-config-validator` (`.github/renovate.json` or `.pre-commit-config.yaml`),
  `packer-validate` (the runner image template) and `runs-on-lint` (`.github/runs-on.yml`); Packer
  and the RunsOn CLI are the `build-config.json` pins, verified and cached in `.cache/tools/`.
  `editorconfig-checker` enforces `.editorconfig`; the gersemi, yamlfmt, and qmlformat formatter
  trials are manual-stage checks ([tools/README.md](../tools/README.md#pre_commitpy)).
  `prek install` also installs a `commit-msg` hook enforcing Conventional Commits;
  `default_stages` keeps the other hooks out of that stage.
- English docs get the Markdown, spelling, and prose hooks (markdownlint, typos, vale) through
  pre-commit like any changed file; when their configuration (`.vale.ini`, `.markdownlint*`,
  `.typos.toml`, `.pre-commit-config.yaml`, `tools/pre_commit.py`) changes, the same run also checks
  every English page with those hooks, so new rules reach unchanged pages. `just lint-docs` runs only
  those hooks locally (`tools/pre_commit.py --docs`). The Docs workflow checks English
  internal links without exemptions before building all locales. Its build sets `BRANCH_NAME` to the
  ref name, which `docs/.vitepress/config.mjs` uses as the site base (`/<branch>/`), matching the
  directory `deploy-docs` publishes to. VitePress is pinned to a 2.0
  prerelease because the stable 1.x dependency chain prevents patched Vite updates; its native theme
  navigation replaces `vp-dynamic-nav`, and `npm run docs:build` runs its Node regression test first.
- Check Links runs lychee on PRs, `master` pushes, and manual dispatch; exact legacy URL exceptions in
  `tools/configs/lychee.toml` expire on their recorded review date (checked by `tools/validate_configs.py`).
- The `qt-translation-placeholders` hook checks new or changed TS messages against the PR merge's
  first parent or the push baseline (prek passes it as `PRE_COMMIT_FROM_REF`) without installing Qt.
  Unchanged legacy placeholder defects do not block unrelated imports; malformed XML and newly
  introduced placeholder mismatches fail. `ci-scripts.yml` runs the validator's tests.
- CodeQL scans Actions, Java/Kotlin, and Python (including `tools/`, `deploy/`, tests, and CI
  scripts); C++ is not scanned by CodeQL. It runs on `ubuntu-latest` with a 10-minute timeout: on
  `ubuntu-22.04` about one PR run in seven failed checkout and then hung in harden-runner's post step
  until the 30-minute timeout cancelled the check.

## Tests

`ci-scripts.yml` runs on changes to `.github/`, `tools/`, `deploy/`, CMake files, and other tooling
configuration:

- the full Python suite (`pytest -q -n auto tools/tests`, including the CI script tests in
  `tools/tests/ci`) on Python 3.10 and 3.12 with the `release` group's git-cliff for the
  release-notes tests (the 3.12 leg adds the `terrain` group so the terrain pipeline tests run),
  plus the tests marked `windows` (`pytest -q -n 2 --durations=15 -m windows tools/tests`:
  platform detection, subprocess, network, file I/O and checksum sidecars, Qt and dependency
  setup, configure, cache planning and artifacts, attestation and AWS upload tests) on Windows,
  with two xdist workers as a trial (revert to serial if CI shows it slower). Linux tests run in
  parallel;
  all run in random order, with a 60 s per-test timeout; a failing run
  prints the `--randomly-seed` that reproduces its order (see
  [`tools/ci/README.md`](../tools/ci/README.md#tests)).

Python and workflow lint run as `pre-commit.yml` hooks: Ruff check and format, Pyright (over the
whole project whenever a Python file changes, so a break in an unchanged module still fails), the
`tools/` import-linter layers (`ci` on top, `common` at the base), `tools/validate_configs.py`, and
actionlint (which reads `.github/actionlint.yml`, listing the repository `vars`) with zizmor.

The test jobs use sparse checkouts. `empty_parameter_set_mark = "fail_at_collect"` in
`tools/pyproject.toml` makes a test that parametrizes over repository files (such as the
`*.VehicleConfig.json` pages) fail collection when it finds none, so add such inputs to both
checkouts and to the workflow's `paths`.

Run the same suite locally with `just test-python`, and the lint hooks with `just lint`.

### Python environment

`setup-python` synchronizes frozen dependency groups from `tools/uv.lock` into `tools/.venv`
(narrower calls keep already-installed groups) and adds that environment to `PATH`. CMake uses the
same interpreter, and MAVLink generation uses its preinstalled dependencies without installing
packages during a build. Ruff, Pyright, clang-format, and import-linter hooks run from the locked
`lint` group, so the hooks and `tools/.venv` share one version.
