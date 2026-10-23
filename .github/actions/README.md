# GitHub Actions

This directory contains the local composite actions shared by QGroundControl workflows. See
[`../ci-overview.md`](../ci-overview.md) for workflow layout and CI conventions and
[`tools/ci/README.md`](../../tools/ci/README.md) for the Python helpers the actions call.

Workflows reference these actions with `uses: ./.github/actions/<name>`, so a job must check out
the action directories (and the `tools/` helpers they run) before using them; sparse checkouts list
them explicitly. The `$/` self-repository syntax is not used: on pull requests from forks the runner
cannot load actions from the PR merge commit (zizmor's `self-repository` audit is disabled for this).

## Action Index

| Action | Purpose |
| --- | --- |
| `android-emulator-test` | Boot a fresh x86_64 AVD, install and launch the APK, and collect failure diagnostics |
| `attest-and-upload` | Checksum, SBOM scan, provenance and SBOM attestations, GitHub artifact upload, and optional AWS publish |
| `aws-credentials` | Configure AWS credentials via OIDC (preferred) or static keys in one step |
| `build-action` | Build a JavaScript action from source when no release ships a rebuilt `dist/` (kept intentionally; no current callers) |
| `build-config` | Expose `.github/build-config.json` values as step outputs |
| `build-setup` | Platform build setup: build config, runner preparation, Python, the CMake and Ninja pinned in `build-config.json`, caches (skipped with `build-cache: 'false'`), Xcode/MSVC, Java and the Android SDK/NDK, and Qt for desktop, Android (each requested ABI), or iOS; ends with the job summary's Setup section |
| `cache` | Plan caches with `cache_plan.py`, install ccache/mold, and restore compiler, moc, CPM, and GStreamer SDK caches (each restore result is an output) |
| `cmake-build` | Build through `cmake_helper.py`, save build caches after success, upload Ninja reports, annotate PR warnings, and write the job summary's Build section |
| `cmake-configure` | Configure QGroundControl from a required CMake preset, which sets the build type and testing; requires `qt-cmake` unless `use-qt-cmake` is `false` |
| `debug-runner` | Open an actor-only SSH session for failed manual runs (callers gate it and set a step timeout) |
| `deploy-docs` | Deploy built documentation to an external GitHub Pages repository |
| `docker` | Build a QGC builder image with Buildx and run the containerized application build |
| `download-all-artifacts` | Download artifacts from every platform workflow run for a commit and optionally record artifact sizes |
| `free-disk-space` | Wrap `endersonmenezes/free-disk-space` with QGC's shared defaults |
| `install-dependencies` | Install platform build dependencies (Linux apt packages, macOS Homebrew and GStreamer, Windows MSVC and NSIS) but not the CMake, Ninja, ccache, and mold that `build-setup` pins; on Linux, `category` installs a single `install_dependencies` category |
| `qt-install` | Reuse a compatible preinstalled Qt SDK or install Qt with QGC's installer or aqtinstall (`installer`, empty for `install_qt.py`'s per-host default), with caching; `version` defaults to `build-config.json`'s; outputs the version, installer, and source |
| `run-unit-tests` | Run CTest, then summarize the JUnit results (counts, failures, slowest tests) and annotate failed tests, upload artifacts, and Codecov test analytics even after failures |
| `save-build-cache` | Save compiler and moc caches once per job after successful compilation |
| `setup-python` | Detect the cache backend, set up uv and Python, and sync `tools/uv.lock` groups into `tools/.venv` |
| `test-phase` | Run the Unit and Integration suites through `run-unit-tests`, one job-summary section each |
| `verify-executable` | Boot-test a built binary, AppImage, DMG, or Windows executable |

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
  (no `checks: write` needed); Codecov test analytics (`upload-codecov`) needs `id-token: write`.
- **`download-all-artifacts`** serves the build-results jobs. PR reporting allows absent
  diagnostic artifacts when tests did not run, but download failures still fail the job;
  baseline downloads allow neither. Baseline
  downloads pass `require-sizes`, which removes `sizes-file` and fails when no distributable
  artifact exists, so an empty baseline is never published.
- **`cache` / `save-build-cache`**: `cache` restores compiler/moc archives and immutable
  CPM/SDK snapshots and exports the save policy; `save-build-cache` checkpoints compiler/moc
  archives once per job after verified success (`cmake-build` calls it for full and `autogen`
  builds; Docker passes its build report).
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
- **`cmake-configure`, `cmake-build`, `test-phase`, `run-unit-tests`** default `build-dir` to
  `${{ runner.temp }}/build`; callers pass it only for another directory. `cmake_helper.py`
  sets `QGC_STABLE_BUILD` for release tags and `Stable*` branches. Callers pass no build type:
  `cmake_helper.py` passes `--config`/`-C` only to multi-config build trees (Xcode for
  `iOS-appstore`), taking the preset's `CMAKE_BUILD_TYPE` from `CMakeCache.txt`.
- **`attest-and-upload`** finds the artifact at `$RUNNER_TEMP/build/<artifact-name>` unless
  `artifact-source-path` overrides it. Pull requests skip the SBOM scan and attestations but
  still upload the checksum, dependency SBOM, and build identity.
