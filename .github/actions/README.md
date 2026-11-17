# GitHub Actions

This directory contains the local actions shared by QGroundControl workflows: composite actions,
plus `disk-watch`, which is JavaScript because it needs a post step. See the
[CI overview](../ci-overview.md) for what runs when and how to change a workflow, the
[CI notes](../ci-notes/) for design rationale, and
[`tools/ci/README.md`](../../tools/ci/README.md#script-index) for the Python scripts the actions
run. Platform jobs typically chain `build-setup`, `install-dependencies`, `cmake-configure`,
`cmake-build`, tests, packaging, `artifact-report`, `verify-executable`, and `attest-and-upload`.

Workflows reference these actions with `uses: ./.github/actions/<name>`, so a job must check out
the action directories (and the `tools/` helpers they run) before using them; sparse checkouts
include `.github/actions`
([rule](../../tools/ci/README.md#shared-code-and-bootstrapping)). The `$/` self-repository syntax
is not used: on pull requests from forks the runner cannot load actions from the PR merge commit
(zizmor's `self-repository` audit is disabled for this).

## Action Index

| Action | Purpose |
| --- | --- |
| `android-emulator-test` | Boot a fresh x86_64 AVD, install and launch the APK, which must run QGC's `--simple-boot-test` to completion, and collect failure diagnostics, linked from the job summary |
| `artifact-report` | Compare a package's contents and size with master's same-named artifact in the job summary (warning beyond `size-threshold` percent) and fail when it ships GPL-licensed GStreamer code, or warn with `gpl-findings: warn` (only the Linux AppImage, a known issue); the job needs `actions: read` |
| `attest-and-upload` | Checksum, SBOM scan, provenance and SBOM attestations, and GitHub artifact upload; `publish` marks the package for the Publish Builds workflow, `companion-path` adds an AppImage's `.zsync`, and the action itself publishes nothing |
| `aws-credentials` | Configure AWS credentials via OIDC (preferred) or static keys in one step; used by the Publish Builds, GStreamer mirror, and runner-image workflows, never by a build |
| `build-action` | Build a JavaScript action from source when no release ships a rebuilt `dist/` (kept intentionally; no current callers) |
| `build-setup` | Platform build setup: `build_setup.py` validates the inputs and plans every mode-dependent value, then runner preparation, Python, the CMake and Ninja pinned in `build-config.json`, caches, Xcode/MSVC, Java and the Android SDK/NDK, and Qt for desktop, Android (each requested ABI), or iOS; ends with the job summary's Setup section, also after a failed step, which it names |
| `cache` | Plan caches with `cache_plan.py`, install ccache/mold, and restore compiler, moc, CPM, and GStreamer SDK caches (each restore result, and the compiler and moc caches' matched key, is an output) |
| `cmake-build` | Build through `cmake_helper.py`, save build caches after success, upload and link Ninja reports, annotate build errors and PR warnings, and write the job summary's Build section |
| `cmake-configure` | Configure QGroundControl from a required CMake preset, which sets the build type and testing, with AppImageLint off (`-DQGC_RUN_APPIMAGELINT=OFF`, ahead of the caller's `extra-args`); requires `qt-cmake` unless `use-qt-cmake` is `false`; writes the job summary's Configure section and, after a failure, annotates the first CMake error and uploads and links the configure logs |
| `debug-runner` | Open an actor-only SSH session for failed manual runs (callers gate it and set a step timeout) |
| `deploy-docs` | Deploy built documentation to an external GitHub Pages repository |
| `disk-watch` | Sample the workspace volume in the background and, from its post step at the end of the job, log and summarize start, peak, and end disk use, warning when the least free space fell below 10% of the volume, kept between 5 and 10 GiB (`build-setup` and `docker` run it first); the only JavaScript action, because composite actions have no post step |
| `docker` | Build (and optionally push) a QGC builder image with Buildx, run the containerized application build in an unprivileged container, and report its timings and cache statistics; outputs the image's local tag |
| `free-disk-space` | Wrap `endersonmenezes/free-disk-space` with QGC's shared defaults |
| `install-dependencies` | Install platform build dependencies (Linux apt packages, macOS Homebrew and GStreamer, Windows MSVC and NSIS; macOS and Windows make three attempts, killing a hung one's process tree) but not the CMake, Ninja, ccache, and mold that `build-setup` pins; on Linux, `category` installs a single `install_dependencies` category |
| `qt-install` | Reuse a compatible preinstalled Qt SDK or install `build-config.json`'s Qt version with QGC's installer or aqtinstall (`QGC_QT_INSTALLER`, else QGC's), with caching; outputs the version, installer, and source |
| `run-unit-tests` | Run CTest, rerunning failed tests once, then summarize the JUnit results (counts, failures, tests passed on retry, slowest tests) and annotate failed tests, upload and link artifacts, and Codecov test analytics even after failures |
| `save-build-cache` | Save compiler and moc caches once per job after successful compilation, as a seven-day snapshot artifact on GitHub storage; release tags save none, and Cache Cleanup deletes superseded snapshots |
| `setup-python` | Detect the cache backend, set up uv, and sync `tools/uv.lock` groups into `tools/.venv` on the Python named in `tools/.python-version` |
| `test-phase` | Run the Unit and Integration suites through `run-unit-tests`, one job-summary section each |
| `verify-executable` | Boot-test a built binary, AppImage, DMG, or Windows executable from its directory (a DMG's bundle is first audited: signature, and every library its Mach-O files load); a failure uploads its diagnostics (7 days), linked from the job summary |

## External Action Reference Policy

Every action and reusable workflow outside `actions/*` and `github/*` is pinned to a full commit
SHA, followed by a comment with its version, such as
`step-security/harden-runner@1d88384fd173af6ecb012dfd0264a306b711241a # v2.22.4`: whoever controls
an action's repository can move a tag, but not change a commit. GitHub's own actions are
referenced by version tag, such as `actions/checkout@v7`. Renovate updates the SHA and the comment
together, adopting a release seven days after it is published
([how, and how to add an action](../ci-notes/dependencies.md#github-actions)).

Three checks enforce the policy. [The zizmor policy](../zizmor.yml) requires the commit SHA
(`hash-pin`) except for GitHub's own actions (`ref-pin`). A contract test
(`test_third_party_actions_are_pinned_to_commits_and_githubs_own_to_version_tags` in
`tools/tests/ci/contracts/test_workflow_security_policy.py`) also requires the version comment,
and a version tag where zizmor accepts any ref, so a branch reference such as
`actions/checkout@main` fails CI. CodeQL's `actions/unpinned-tag` query reports a third-party
action that a workflow references by tag.

Automated findings whose only concern is that an `actions/*` or `github/*` action is referenced by
a released version tag instead of a full commit SHA are accepted under this repository policy and
do not require a code change. Reference this section when dismissing or responding to those
findings. This exception does not apply to third-party actions, unversioned actions, branch
references such as `@main`, unknown actions, or any warning that reports an additional security
problem.

## Action Notes

- **`test-phase`** runs the Unit suite, then the Integration suite even if Unit failed, and
  writes a job-summary section for each (counts, failures, slowest tests) and warns about tests
  slower than 60 s. Callers must gate it on a successful build. Integration tests default to two CTest processes (`integration-parallel`); Unit tests use
  one per CPU.
- **`run-unit-tests`** runs `cmake_helper.py ctest` (`--no-tests=error`, so empty selections
  fail), selecting by labels and, with `tests-regex`, by test name (`-R`). It reruns failed tests
  once; a test that then passes is reported as "passed on retry" and no longer fails the step
  ([when it retries](../../test/README.md#ci-retries-and-the-flaky-label)). Reporting steps run
  after test failures; the failed CTest step still fails the job.
  `junit_report.py` writes the `report-name` section and error annotations for failed tests
  (no `checks: write` needed), and `artifact_link.py` links the uploaded results below it; Codecov
  test analytics (`upload-codecov`) needs `id-token: write`.
- **`build-setup`** starts with `build_setup.py` on the system Python: it validates the mode's
  inputs, creates `$RUNNER_TEMP/build`, and outputs the Qt hosts, architectures, and modules, the
  cache target, Xcode and MSVC selections, the Android ABIs to install, and the `build-config.json`
  versions that later steps and their `if:` conditions read; Android jobs also get
  `ANDROID_MIN_SDK` and, off macOS, an SDK root under `$RUNNER_TEMP`.
- **`qt-install`**: `install_qt.py plan` resolves the request once and outputs the cache key,
  whether to save (never for a reused preinstalled SDK, a pull request from a fork, or a Copilot
  session), and the
  request as JSON; `ensure --plan` reuses or installs exactly that request and reports its source.
- **`cache` / `save-build-cache`**: `cache` restores compiler/moc archives and immutable
  CPM/SDK snapshots and exports the save policy; `save-build-cache` checkpoints compiler/moc
  archives once per job after verified success (`cmake-build` calls it for full and `autogen`
  builds; Docker passes its build report). Keys, save gates, the snapshot artifacts, and their
  retention are described in [caching.md](../ci-notes/caching.md#compiler-and-moc-caches).
  A release tag restores and saves neither compiler nor moc archives
  ([caching.md](../ci-notes/caching.md#release-tags)).
- **`cmake-build`** writes per-invocation Ninja reports and uploads `ninja.log`, `report.json`,
  and `report.md` even after compilation fails. Reports separate moc/code generation and linking,
  deduplicate commands with multiple outputs, and exclude earlier builds in the same directory.
  Docker includes the same reports in its performance artifact; non-Ninja generators skip them.
  The Build step itself writes the job summary's Build section, also after failures but not when
  the job is cancelled, and `artifact_link.py` links the uploaded reports below it. The step tees
  its output to `output-file` (default `$RUNNER_TEMP/build-output.log`); after a failure,
  `build_errors.py` finds the Ninja (or Xcode) failed targets and the first ten distinct CMake,
  GCC/Clang/javac, MSVC, linker, and Gradle errors, which become `file`/`line`/`col` error
  annotations (paths relative to the checkout) and the Build section's collapsed
  "Failed targets / first errors". The compiler-cache rows count only this build's requests, so a
  later `qgc-package` (Windows) or `aab` (Android) build does not repeat the main build's hits.
  Reviewdog annotates warnings only after a successful build.
- **`cmake-configure`** tees CMake's output to `cmake-configure.log` in the build directory and
  writes the job summary's Configure section: result, duration, preset, generator, and
  `CheckResultCache.cmake`'s configure-check reuse. After a failure the first `CMake Error` block
  becomes an error annotation titled "CMake configure failed" (at its file and line when that is
  in the checkout) and a collapsed block with the end of the output, and the step uploads
  `cmake-configure.log`, `CMakeCache.txt`, and CMake 4's `CMakeFiles/CMakeConfigureLog.yaml` for
  seven days, linked below the section.
- **`cmake-configure`, `cmake-build`, `test-phase`, `run-unit-tests`** default `build-dir` to
  `${{ runner.temp }}/build`; callers pass it only for another directory. `cmake_helper.py`
  sets `QGC_STABLE_BUILD` for release tags and `Stable*` branches. Callers pass no build type:
  `cmake_helper.py` passes `--config`/`-C` only to multi-config build trees (Xcode for
  `iOS-appstore`), taking the preset's `CMAKE_BUILD_TYPE` from `CMakeCache.txt`.
- **`attest-and-upload`** finds the artifact at `$RUNNER_TEMP/build/<artifact-name>` unless
  `artifact-source-path` overrides it. Pull requests skip the SBOM scan and attestations but
  still upload the checksum, dependency SBOM, and build identity. `attest_helper.py prepare`
  resolves the attestation subject (`subject-name`, defaulting to `package-name`). Every caller
  sets `publish`: `'true'` adds the publish marker to the build identity (`<file>.build.json`),
  which tells the Publish Builds workflow to publish the package as `artifact-name`
  ([how](../ci-notes/platforms.md#publish-builds)); the step fails unless `PUBLISHED_BUILDS`
  lists that name for the workflow. A caller that can mark a package also passes
  `job-id: ${{ job.check_run_id }}`, the job the publisher looks up. Without a usable id the
  step warns and the build passes; the publisher then refuses the package in its own run.
  `companion-path` names a file that goes beside the package on its GitHub Release and nowhere
  else: the `.zsync` that appimagetool writes for an AppImage with update information, which
  must be `<artifact-name>.zsync` in the package's directory (the step fails for anything
  else). It is uploaded in the artifact and listed, with its SHA-256, in the publish marker, so
  the publisher requires it. The provenance attestation covers the package, the build identity,
  and the companion file, so the publisher verifies each. The action takes no AWS credentials
  and uploads nothing outside GitHub.
- **`verify-executable`** outputs a per-package diagnostics directory and artifact name before
  booting (job, runner OS and architecture, package, a digest of its path, and run attempt, so
  several boot tests in one job or matrix never collide). A failed boot keeps the output there;
  a timeout first captures stacks (`sample` on macOS, `gdb` or `eu-stack` through passwordless
  `sudo` on Linux, a minidump on Windows); macOS adds the run's `DiagnosticReports` crash reports
  and prints the newest one's first 60 lines; Windows sets `QGC_CRASH_DUMP_DIR` there for QGC's
  own crash minidump and adds the executable's PDB beside it (MSVC Release links write one, see
  `cmake/Toolchain.cmake`). The directory is
  uploaded for seven days only after a failure and linked below the Boot test section, which
  also shows the last log line and the last 40 lines of output.
- **`android-emulator-test`** links its `emulator-diagnostics-*` artifact from the job summary
  after a failure.
