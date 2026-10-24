# GitHub Actions

This directory contains the local actions shared by QGroundControl workflows: composite actions,
plus `disk-watch`, which is JavaScript because it needs a post step. See
[`../ci-overview.md`](../ci-overview.md) for workflow layout and CI conventions and
[`tools/ci/README.md`](../../tools/ci/README.md) for the Python helpers the actions call.

Workflows reference these actions with `uses: ./.github/actions/<name>`, so a job must check out
the action directories (and the `tools/` helpers they run) before using them; sparse checkouts list
them explicitly. The `$/` self-repository syntax is not used: on pull requests from forks the runner
cannot load actions from the PR merge commit (zizmor's `self-repository` audit is disabled for this).

## Action Index

| Action | Purpose |
| --- | --- |
| `android-emulator-test` | Boot a fresh x86_64 AVD, install and launch the APK, and collect failure diagnostics, linked from the job summary |
| `attest-and-upload` | Checksum, SBOM scan, provenance and SBOM attestations, GitHub artifact upload, and optional AWS publish |
| `aws-credentials` | Configure AWS credentials via OIDC (preferred) or static keys in one step |
| `build-action` | Build a JavaScript action from source when no release ships a rebuilt `dist/` (kept intentionally; no current callers) |
| `build-setup` | Platform build setup: `build_setup.py` validates the inputs and plans every mode-dependent value, then runner preparation, Python, the CMake and Ninja pinned in `build-config.json`, caches (skipped with `build-cache: 'false'`), Xcode/MSVC, Java and the Android SDK/NDK, and Qt for desktop, Android (each requested ABI), or iOS; ends with the job summary's Setup section, also after a failed step, which it names |
| `cache` | Plan caches with `cache_plan.py`, install ccache/mold, and restore compiler, moc, CPM, and GStreamer SDK caches (each restore result, and the compiler and moc caches' matched key, is an output) |
| `cmake-build` | Build through `cmake_helper.py`, save build caches after success, upload and link Ninja reports, annotate build errors and PR warnings, and write the job summary's Build section |
| `cmake-configure` | Configure QGroundControl from a required CMake preset, which sets the build type and testing; requires `qt-cmake` unless `use-qt-cmake` is `false`; writes the job summary's Configure section and, after a failure, annotates the first CMake error and uploads and links the configure logs |
| `debug-runner` | Open an actor-only SSH session for failed manual runs (callers gate it and set a step timeout) |
| `deploy-docs` | Deploy built documentation to an external GitHub Pages repository |
| `disk-watch` | Sample the workspace volume in the background and, from its post step at the end of the job, log and summarize start, peak, and end disk use, warning when the least free space fell below 10% of the volume, kept between 5 and 10 GiB (`build-setup` and `docker` run it first); the only JavaScript action, because composite actions have no post step |
| `docker` | Build (and optionally push) a QGC builder image with Buildx, run the containerized application build, and report its timings and cache statistics; outputs the image's local tag |
| `free-disk-space` | Wrap `endersonmenezes/free-disk-space` with QGC's shared defaults |
| `install-dependencies` | Install platform build dependencies (Linux apt packages, macOS Homebrew and GStreamer, Windows MSVC and NSIS; macOS and Windows make three attempts, killing a hung one's process tree) but not the CMake, Ninja, ccache, and mold that `build-setup` pins; on Linux, `category` installs a single `install_dependencies` category |
| `qt-install` | Reuse a compatible preinstalled Qt SDK or install `build-config.json`'s Qt version with QGC's installer or aqtinstall (`QGC_QT_INSTALLER`, else QGC's), with caching; outputs the version, installer, and source |
| `run-unit-tests` | Run CTest, then summarize the JUnit results (counts, failures, slowest tests) and annotate failed tests, upload and link artifacts, and Codecov test analytics even after failures |
| `save-build-cache` | Save compiler and moc caches once per job after successful compilation |
| `setup-python` | Detect the cache backend, set up uv and Python, and sync `tools/uv.lock` groups into `tools/.venv` |
| `test-phase` | Run the Unit and Integration suites through `run-unit-tests`, one job-summary section each |
| `verify-executable` | Boot-test a built binary, AppImage, DMG, or Windows executable from its directory; a failure uploads its diagnostics (7 days), linked from the job summary |

## External Action Reference Policy

QGroundControl intentionally references external GitHub Actions by stable major-version tag, such as
`actions/checkout@v7`, instead of pinning each action to a full commit SHA. Dependabot monitors these
references and proposes action updates, while [the zizmor policy](../zizmor.yml) requires a tag or
other ref pin and rejects unpinned branch references.
CodeQL excludes `actions/unpinned-tag` to match this policy; other security queries remain enabled.

Automated findings whose only concern is that a released action tag is mutable or is not a full
commit SHA are accepted under this repository policy and do not require a code change. Reference
this section when dismissing or responding to those findings. This exception does not apply to
unversioned actions, branch references such as `@main`, unknown actions, or any warning that reports
an additional security problem.

## Action Notes

- **`test-phase`** runs the Unit suite, then the Integration suite even if Unit failed, and
  writes a job-summary section for each (counts, failures, slowest tests) and warns about tests
  slower than 60 s. Callers must gate it on a successful build. Integration tests default to two CTest processes (`integration-parallel`); Unit tests use
  one per CPU.
- **`run-unit-tests`** runs `cmake_helper.py ctest` (`--no-tests=error`, so empty selections
  fail). Reporting steps run after test failures; the failed CTest step still fails the job.
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
  builds; Docker passes its build report). Its `ccache_helper.py save-plan` step gates PR saves,
  packs the snapshot, and outputs the cache keys to save (empty skips a cache), even when the
  gate fails; the cache saves run only if no snapshot upload succeeded.
- **Compiler snapshots (GitHub storage only)**: `build_cache_artifact.py` keeps compiler/moc
  snapshots as uncompressed one-day workflow artifacts, outside the dependency-cache quota, so
  concurrent SDK and dependency cache writes cannot evict a just-completed build's objects.
  Restores accept the same workflow, source repository, branch, and event class; new branches
  can also use a trusted default-branch snapshot. Names include platform, build type, variant,
  key suffix, and branch scope, but not the config hashes in the dependency-cache keys
  (content-addressed entries stay valid across those edits). Pull requests restore the default
  branch's snapshot without publishing their own. Any snapshot failure falls back to the
  dependency cache; after expiry a cold build is possible. RunsOn/S3 keeps its cache/save route.
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
  resolves the attestation subject (`subject-name`, defaulting to `package-name`) and whether to
  publish to AWS (`upload-aws`, credentials present, and a push or tag dispatch in `mavlink`).
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
