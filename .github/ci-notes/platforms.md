# CI Notes: Platforms and Workflows

Design notes on individual workflows: how change detection decides what builds, compiler and test
settings, per-platform build, packaging and signing specifics, and the checks that are not
platform builds. The [CI overview](../ci-overview.md) has the contributor-facing summary.

## Change Detection

- Platform workflows (Linux, macOS, Windows, iOS, Android, Docker) call `_detect-changes.yml`
  (job `changes / Detect Changes`): one sparse checkout and one `plan_platform.py --platform <name>`
  call that runs `detect_changes.detect_builds()` once and outputs `should_build` and `matrix`.
  For Android (`android_matrix.py`) and Docker (`plan_docker_builds.py`), `matrix` is the JSON
  include-list of build legs and `should_build` is whether it has any; other platforms get `[]`
  and define their own matrices.
- Pull requests build a platform only when its inputs changed (patterns in `detect_changes.py`).
  Paths that no platform build runs or reads are dropped first (`_NOT_BUILD_INPUTS`): every
  Markdown file, and the developer, lint, analysis, reporting, and maintenance scripts under
  `tools/`. Any other path under `tools/` builds every platform, except `tools/tests/`,
  `tools/configs/` other than `ccache.conf`, and `tools/setup/`, whose installers build the
  platforms they serve. A new script is therefore a build input until `_NOT_BUILD_INPUTS` names
  it, and `tools/tests/ci/test_detect_changes.py` fails when a script that a platform workflow
  runs or imports is listed there.
- A pull request that changes only `tools/pyproject.toml` or `tools/uv.lock` (Renovate's lock
  updates) builds Linux alone, as the canary for the other platforms; the Dev Container workflow's
  pull request filter leaves those two files out for the same reason.
- Application sources, tests, resources, and translations build every native platform. The Docker
  builder images do not depend on them, so Docker's pull request legs build only for the CMake
  files among them (`CMakeLists.txt` and `*.cmake`).
- Pushes, merge queues, and dispatches build everything (a push that changes only `docs/**` starts
  no platform workflow); tags follow [Release tags](#release-tags).
- **Gate jobs**: Linux, Windows, MacOS, Android, and iOS end with an `if: always()` gate job named
  after the workflow (`re-actors/alls-green`) that fails if change detection or any build leg
  fails or is cancelled; builds may be skipped only by change detection. A ruleset that requires
  platform builds should name these five checks, not build jobs: a skipped matrix job reports its
  unexpanded name (for example `Release ${{ matrix.arch }}`), and Android's legs differ by event.
  `test_required_check_gates.py` pins the names and checks that each gate needs every other job
  in its workflow. Which checks are required is a ruleset setting outside the repository;
  upstream's rulesets require none today (read on 2026-11-12).
- PR files come from `common.git.pr_diff_refs`, as in pre-commit and Code Analysis: the detection
  checkouts fetch depth 2 and compare GitHub's merge commit with its first parent, the base tip it
  merged into (`pull_request.base.sha` can be older and would add newer base-branch changes), so a
  PR that is behind its base branch does not build for the base branch's newer changes. Files are
  listed with `git diff --no-renames`: a moved file counts at both paths, so moving it out of a
  watched directory such as `src/` still triggers the builds that used it. If the diff can't be
  computed, builds run anyway.
- Docker PRs build Ubuntu 24.04 plus Android when affected (PRs touching only `android/` or
  `deploy/android/` run Docker Android). Toolchain, dependency, and packaging changes, unknown
  diffs, pushes, and dispatches build every variant. The full-matrix list in
  `plan_docker_builds.py` names Docker actions, scripts, dependency installers, and build inputs
  explicitly; unrelated GitHub metadata, analyzer scripts, tool tests, and VM provisioners do not
  expand the matrix.

## Compiler Settings

- **Unity builds**: Linux coverage and sanitizer builds and the iOS simulator build inherit the
  hidden `lean-debug` preset (`-g1` plus `QGC_UNITY_BUILD=ON`); the macOS Debug leg passes
  `-DQGC_UNITY_BUILD=ON`. Unity batches halve compile CPU (measured for pull request 15370,
  October 2026). Bundled third-party code, the GPS
  protocol and GStreamer libraries, and files marked `SKIP_UNITY_BUILD_INCLUSION` stay per-file;
  Release jobs stay per-file. A redefinition or ambiguity error means two `.cc` files in one batch
  share file-local names or a file-scope `using namespace`: rename them or mark the file, and
  reproduce with `-DQGC_UNITY_BUILD=ON`. Apple builds batch a different source list than Linux, and
  adding or removing a source shifts later batches, so such errors can appear in files a PR did not
  touch (and the shifted batches miss the compiler cache once).
- **Line-table debug info** (`-g1`) is all gcov and sanitizer stacks need: it cuts objects by ~70%
  and compile CPU by ~17% against full `-g` (measured for pull request 15323, October 2026), and
  keeps the iOS simulator objects within the compiler-cache budget.
- **rcc launcher**: CMake routes `Qt6::rcc` through a launcher that pins the file timestamps rcc
  embeds, so fresh checkouts keep generated `qrc_*.cpp` objects cacheable (an exported
  `QT_RCC_SOURCE_DATE_OVERRIDE` or `SOURCE_DATE_EPOCH` still wins), and lowers rcc's default zstd
  level from 19 to 9 (about 10x faster for ~2% more resource data, measured for pull request
  15323, October 2026) when QtCore supports zstd.
- **CMake entrypoint**: `cmake-configure` requires `qt-cmake` by default. Android is the exception
  and supplies its target Qt toolchain and prefix to plain CMake. Every caller names a configure
  preset, which sets the build type and `QGC_BUILD_TESTING`. A failed configure uploads
  `cmake-configure.log`, `CMakeCache.txt`, and `CMakeFiles/CMakeConfigureLog.yaml` (CMake 4 no
  longer writes `CMakeOutput.log` or `CMakeError.log`).
- **Android javac warnings**: `setup-java`'s `javac` problem matcher made each deprecation warning
  in the Gradle build an annotation; `android.yml` removes it after `build-setup`, and `cmake-build`
  annotates javac errors itself.
- **Windows Qt architecture**: aqt architecture names are explicit package identifiers;
  `test_cmake_presets.py` requires every workflow matrix to use the same MSVC generation.
- **Windows linker**: Debug dispatches of `windows.yml` pass `QGC_USE_LLD_LINK=ON` to link with
  `lld-link` when the runner has it. Release builds keep `link.exe`: lld-link cannot link the IPO
  (`/GL`) objects they use.

## Tests

How CI runs, retries, and reports the application tests is described in
[test/README.md](../../test/README.md#running-tests) and
[reporting.md](reporting.md#test-results-and-coverage); these are the per-platform specifics.

- `test-phase` runs the Unit and Integration suites for Linux Debug (coverage and sanitizers) and
  macOS Debug. Call it only after a successful build. Unit tests run one CTest process per CPU,
  including coverage (GCC merges `.gcda` counters under file locks). Integration tests use two
  processes by default and for coverage, where instrumented tests otherwise hit their timeouts;
  Linux sanitizers use one per CPU.
- **Boot test**: `QGroundControl --simple-boot-test`, which every platform's package runs, also runs QGC's
  GStreamer self-test: H.264 (and, given a decoder, H.265) decoding, MP4 and MKV recording, every feature's
  elements, and for a bundled runtime its plugins, plugin scanner and GIO TLS/proxy modules
  ([details](../../src/VideoManager/GStreamer/README.md#boot-self-test)).
- **Sanitizers**: `Linux-sanitizers` (ASan + UBSan, leak detection on) excludes the `NoSanitizer`
  label. Runtime options come from CTest: `cmake/QGCTest.cmake` sets `*SAN_OPTIONS`
  (`halt_on_error=1`) on every test, overriding the shell environment. ThreadSanitizer runs only on
  manual Code Analysis dispatch.
- **Coverage**: manual Linux coverage dispatches also run the normally excluded `Network|Flaky`
  tests serially against the same build after the coverage report; they have separate reports and
  still fail the job.
- Reproduce either Linux lane with `cmake --workflow --preset Linux-sanitizers` or
  `--preset Linux-coverage` (`just coverage` uses `Linux-coverage`).
- **Portable tests**: Windows x64 and the macOS Release leg build and run ten standalone portable
  utility executables (`QGC_BUILD_PORTABLE_TESTS=ON`, CTest label `Portable`). They share test
  bodies with the full harness and do not enable test hooks in the packaged application. There is
  no separate Extended Tests workflow.
- **Windows GStreamer tests**: `windows.yml`'s `gstreamer-tests` job builds x64 Debug with tests
  and runs only the GStreamer and video test classes and `cmake/GStreamer`'s script tests
  (`run-unit-tests`' `tests-regex`
  `^(Gst|GStreamer[A-Z.]|Video|Subtitle|QGCVideo|QtMultimedia|QtVideo|UVC|StreamSupervisor|RecordingPlan)`, Unit and
  Integration), with the Qt Multimedia receiver, its recorder, the USB camera classes and the reconnect and
  recording-part policies both receivers share, so the script tests' Windows-only branches (8.3 short paths, SDK
  architecture) run on Windows. The build tree finds the
  SDK only through `verify_executable.py --export-gstreamer-env`, which puts its `bin/` on `PATH`
  and stages the shipped plugins plus the sources and encoders the tests send streams with
  (`videotestsrc`, `mpegtsmux`, `x264`, `x265`). `QSG_RHI_PREFER_SOFTWARE_RENDERER=1` makes the
  D3D11/D3D12 pixel tests create their QRhi on WARP, as the runners have no GPU; the job is
  behind the Windows gate like the build legs, since WARP renders deterministically and a test
  whose device or element is missing skips.

## Platform Notes

- **macOS**: the Debug leg builds for arm64 only and runs the Unit and Integration suites; only the
  universal (x86_64h + arm64) Release leg packages and uploads the DMG. The job's `MACOS_SIGN`
  gates signing and notarization (CMake option, certificate import, and secrets) to non-PR Release
  legs in `mavlink` repositories or forks that set `MACOS_SIGNING_IDENTITY`; other forks build an
  unsigned DMG instead of failing in `SignMacBundle.cmake`.
  `verify_executable.py --type dmg` mounts the image, audits and boots the bundle, and always
  detaches it. The audit (`macho_bundle_audit.py`) runs `codesign --verify --deep --strict` on a
  Developer ID signed bundle (PR builds are ad-hoc signed and only get a notice) and reads every
  Mach-O file's load commands with `otool -l`: each library must resolve inside the bundle,
  through @rpath (the file's and the main executable's LC_RPATHs), @loader_path or
  @executable_path, or to /usr/lib or /System; absolute paths elsewhere (Homebrew,
  /Library/Frameworks/GStreamer.framework, the build machine) fail it, as do such LC_RPATHs where
  dyld searches them. A failed audit fails the step after the boot test.
- **Boot-test diagnostics** (every desktop platform): on a boot-test timeout `verify_executable.py`
  captures the hung process's stacks before killing it and prints them after the captured output,
  and its `lipo`/`hdiutil`/AppImage helper calls time out after 120 s. The boot test forces QGC's
  log to stderr (a Windows GUI app otherwise logs to the debugger) and enables the startup
  categories with `--logging`, so a slow, hung or crashed boot shows where it stopped; a passing
  boot folds that log into a collapsed group. A crash is named by its signal or NTSTATUS
  (`-11 SIGSEGV`, `0xC0000005 STATUS_ACCESS_VIOLATION`) and the step exits 1: Python before 3.13
  cannot exit with a status above 2^31 on Windows, and MSYS bash reported the result as 127. On
  Windows QGC's unhandled-exception filter prints the code, address, and module+RVA to stderr and,
  with `QGC_CRASH_DUMP_DIR` set, has a thread started at launch write a minidump there (not the
  crashed thread, which may hold the loader lock or have no stack left), renamed from `.part` only
  when complete; failure diagnostics are uploaded as described in
  [`actions/README.md`](../actions/README.md#action-notes). On Windows a build tree boots with only
  the GStreamer plugins the package ships (configure records them as `QGC_GST_SHIPPED_PLUGINS`;
  `verify_executable.py` copies them to `boot-test-gstreamer-plugins/`): the whole SDK's ~270
  plugins took ~40 s to scan and hung or crashed QGC on exit (seen in pull request 15370's
  Windows runs, October 2026).
- **iOS**: `ios.yml` builds a Release device bundle and a Debug x86_64 simulator bundle, installing
  Qt's separate `ios_device` and `ios_simulator_x86_64` SDKs. The simulator stays on x86_64 because
  Qt's bundled FFmpeg has no arm64 simulator slice. Simulator builds run `ios_boot_test.py`, which
  launches with `--simple-boot-test` and requires QGC's success marker. Cold boots have a
  600-second deadline and at most two fresh-device attempts (`--boot-timeout` 1–900 s,
  `--boot-attempts` 1–2). Boot and installation failures, and a launch that times out before QGC
  prints anything, are retried on a fresh device; QGC failures are not. Install and launch each
  have a 600-second deadline (`--timeout`), and every attempt's simulator commands share a
  1500-second budget (`--total-timeout`) that ends them before the step's 30-minute timeout. A
  shutdown/delete failure after a passing test is reported as a warning and does not fail the job;
  after a failed attempt it prevents the retry. Each attempt keeps command output; a failed attempt
  in any phase saves the simulator state, the last 10 minutes of the simulator's and
  CoreSimulator's logs, and QGC's new crash reports beside the log before cleanup. A failure ends
  in an error annotation naming the phase and attempt; every run writes a Boot test summary
  section, and the `ios-boot-test` artifact is linked below it.
- **Windows**: installer verification lives in `deploy/windows/verify-installer.ps1`. Between its
  install phase and the installed boot test, `pe_dependency_audit.py` reads the import and
  delay-load tables of every .exe and .dll in the install (a small PE parser, no `pefile`) and
  names each imported DLL that is neither in `bin/` or beside the importer nor in System32; the
  MSVC runtime counts only from `bin/`, as a clean Windows lacks it. Off Windows it checks a
  built-in list of system DLLs, so it can audit an installer extracted with `7z x`.
- **Linux AppImage**: `linux.yml`'s `appimage-distros` job boots the x86_64 AppImage
  (`appimage_container_boot.py`: `--appimage-extract-and-run` under xvfb-run as an unprivileged
  user, no `GST_PLUGIN_*` or `GST_REGISTRY*` variables) in Ubuntu 24.04 and Debian 13 containers,
  the oldest releases its glibc 2.38 and libstdc++ requirements allow. They hold only the
  desktop libraries the AppImage leaves to the system (X11, OpenGL/EGL, GBM, Wayland client,
  fontconfig, HarfBuzz, which brings GLib, ALSA, com_err, libgpg-error) and CA certificates, with
  no GStreamer, Qt or GSettings schemas, so it must bring its own and must not need the schemas.
  The images come from Docker Hub, not the ECR Public mirror: the job runs on a GitHub-hosted
  runner, which Docker Hub's pull limit exempts and whose shared IPs exhaust ECR Public's
  anonymous data quota.
- **AppImageLint is off in CI**: `cmake-configure` prepends `-DQGC_RUN_APPIMAGELINT=OFF` (a caller's
  `extra-args` could turn it back on; none does), and the Docker and VM builds pass it too
  (`deploy/docker/entrypoint.py`, `deploy/multipass/build_in_vm.py`). Its distribution check only
  warns, and its Launchpad lookup cost about nine minutes whenever that host was unreachable
  (seen in pull request 15452's Linux runs, November 2026); the `appimage-distros` job boots the
  AppImage on the oldest supported distributions instead.
- **Linux translation extraction**: `linux.yml`'s coverage leg runs
  `tools/translations/qgc_lupdate.py --check` (`just translations --check`) after its tests and
  coverage steps, whatever their result, so a failure never hides those reports. Qt's `lupdate`
  extracts `src/` into a scratch catalog, and the step fails on a `tr()` call that `lupdate`
  rejects or cannot extract: `tr()` in a class with neither `Q_OBJECT` nor
  `Q_DECLARE_TR_FUNCTIONS`, `tr()` outside a class, or `tr()` called through an object. The weekly
  `lupdate.yml` run fails on the same calls only after they merge, and opens no update pull request
  until they are fixed. The check runs in the Linux workflow because its jobs already install Qt
  with `lupdate` and run for every pull request that changes `src/`; the coverage leg publishes no
  packages. It writes nothing under `translations/`.
- **Android**: legs are Linux (primary), macOS, Windows (not on PRs), and a Linux emulator smoke
  test. PRs build `arm64-v8a` only; other events build `arm64-v8a` + `armeabi-v7a` on the primary
  leg. Android jobs install only the `core` apt category. Every leg checks its APK
  (`android_apk_check.py`, the NDK's llvm-readelf on each `lib/<abi>/*.so`): on the 64-bit ABIs
  every `PT_LOAD` segment must be aligned to 16 KB, which 16 KB-page devices and Google Play need;
  every `DT_NEEDED` library must be packaged for the same ABI or be an NDK system library at the
  minimum API level (the sysroot's stubs); `libgstreamer_android.so` must be packaged when GStreamer
  is enabled; and `zipalign -c -P 16 -v 4` must pass (skipped with a notice by a zipalign without
  `-P`). Problems become error annotations and an APK check job-summary section. The emulator
  leg's APK, which is not published, runs QGC's simple boot test at every launch: its configure
  loads `android-emulator-test`'s `boot-test.cmake` initial cache (`QT_ANDROID_APPLICATION_ARGUMENTS`
  `--simple-boot-test --logging:Main,...`; a release APK ignores `am start` arguments, and
  `extra-args` cannot hold a space). `android_boot_test.py` grants the app all-files access before
  launching it (QGC otherwise opens the system settings over its window and the boot test never
  completes) and passes only on QGC's `Simple boot test completed` marker; a crash, a process exit
  or a running app without the marker fails, and the failure annotation quotes the
  `Simple boot test failed` and `Video.GStreamer.SelfTest` logcat lines. After the marker, main()
  returns, Android destroys the activity and Qt calls `exit()`; the script watches logcat until the
  app's process (the marker line's pid) exits, for up to 30 s (`--exit-timeout`; a process still
  running then only warns). A crash of that process (its `Fatal signal`, `FORTIFY` or
  `FATAL EXCEPTION` lines, or crash_dump's naming it) fails the step without another attempt, and
  the annotation quotes those lines: libhwui's static destructors aborted QGC that way at exit until
  `Platform::skipStaticDestructorsAtExit` ended the process before they ran. The Linux leg runs Android Lint
  (`android_lint.py`, `gradlew lintRelease` in `android-build`): up to ten Error and Fatal issues
  from lint's XML report become error annotations titled with the issue id, at the `android/`
  source that androiddeployqt copied (`QGC_ANDROID_PACKAGE_SOURCE_DIR`; a copy that differs, like
  the rewritten `AndroidManifest.xml`, keeps the file but not the line), or Gradle's errors when
  lint wrote no report; an Android Lint job-summary section follows the Build section, and a
  failure uploads the HTML and XML reports and the Gradle log for seven days, linked below it.
  Master pushes also build a signed multi-ABI App Bundle on the primary leg through `cmake-build`
  (`target: aab`, logged to `qgc-aab-build.log` with its own "Build (aab)" section and failure
  annotations; it saves no build caches), uploaded as its own attested GitHub artifact
  (`QGroundControl-linux-<abis>-aab`); it is neither marked for publishing nor sent to the Play
  Store.
- **Custom plugin** (`linux.yml`'s `custom-plugin` job, behind the Linux gate): the `custom-example`
  overlay changes the app name and feature set, global compile definitions, so it is its own build:
  one Debug build that runs the Unit suite (excluding `StockUI`) and boots the build tree; the stock
  Linux legs run the integration suites and cover Release compiles. Outside pull requests it also
  installs the build stripped (`cmake --install --strip`; the Debug binary carries ~800 MB of debug
  info, measured for pull request 15375, October 2026) into an AppImage and boots that. `custom-example/` changes trigger the Linux workflow.
- **ClusterFuzzLite**: PR runs use the bundled seed corpus without querying historical artifacts
  (`NO_CLUSTERFUZZ_DEPLOYMENT=true`), which also disables previous-build crash comparison:
  reproducible crashes fail the PR whether or not they predate it. Crash files and SARIF
  diagnostics are uploaded separately. The workflow runs only on pull requests: nothing reads
  uploaded fuzzer builds, so there is no continuous build. The base image's own Python serves
  OSS-Fuzz's tooling; QGC's two scripts run on the pinned Python, which uv installs in the image.

## Docker

- One multi-stage `deploy/docker/Dockerfile` builds every variant in `deploy/docker/variants.json`.
  Base images come from the ECR Public mirror of Docker's official images
  (`public.ecr.aws/docker/library/<image>:<tag>@<digest>`, same digests as Docker Hub), so PR builds,
  which don't log in to Docker Hub, avoid its anonymous pull limit. Buildx's `moby/buildkit` image
  and the `docker/dockerfile:1` frontend still come from Docker Hub. No QEMU: every target builds
  for the x86_64 runner, and aarch64 cross-compiles; `devcontainer.yml` builds the `devcontainer`
  target natively on arm64 runners too.
- Each job uploads `docker-build-report.json` (configure/build/package timings, cache statistics,
  CPM source size) with its Ninja build profile. Checkout keeps full history for package
  versioning; checkouts took 13–22 s when measured for pull request 15102 (September 2026), which
  doesn't warrant a repository mirror. The
  application build's caches are described in [caching.md](caching.md#other-caches).
- The build container runs unprivileged: as the calling user, with no added capability, device, or
  security option (`test_deploy_scripts.py::test_docker_cli_uses_variants_without_shell_evaluation`).
  `deploy/docker/entrypoint.py` sets `APPIMAGE_EXTRACT_AND_RUN=1`, so linuxdeploy and appimagetool
  unpack themselves instead of mounting through FUSE. AppImageLint, which has to mount the AppImage
  it checks, is a local check for packaging outside a container.
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

## Dev Container

`devcontainer.yml` builds the Dockerfile's `devcontainer` target natively for amd64 and arm64 and
smoke-tests it (`deploy/docker/smoke_devcontainer.py` builds and analyzes a small Qt program, and
on amd64 compiles it for Android), then opens it through `devcontainer.json` with
`devcontainers/ci` to check its features and `postCreateCommand`; pushes and manual runs also
Grype-scan the image. Upstream `master` pushes that change its inputs publish
`ghcr.io/mavlink/qgc-dev:latest`, release tags [on a `Stable*` branch](#release-tags) publish their
version (with provenance and per-platform SBOM attestations), and pull requests, forks, and manual
runs only build. `.devcontainer/devcontainer.json` reuses the published layers as a build cache.

## VM Builds

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
  Vagrant). Cleanup only deletes instances it created. The job stays `continue-on-error`. A
  job that is `continue-on-error` still shows a failed check on the pull request, so an
  unreachable snap store is not left to fail it: when `snap install` or `snap refresh` fails three
  times, `install_multipass.py --allow-store-outage` warns and reports `installed=false`, and the
  build and upload steps are skipped. Every other failure still fails the job
  (`tools/tests/ci/test_install_multipass.py::test_a_snap_store_outage_fails_unless_the_job_allows_it`,
  `tools/tests/ci/test_install_multipass.py::test_the_multipass_job_builds_only_when_multipass_was_installed`).
- **Vagrant** runs `deploy/vagrant/run_vagrant.py` (`install`, then `build`): `vagrant up` stops
  after 150 minutes (`QGC_VM_UP_TIMEOUT`), the VM is destroyed only after an attempted startup, and a
  failed destroy fails the job; Vagrant failures fail the workflow. Reproduce locally with
  `uv run --frozen --project tools python deploy/vagrant/run_vagrant.py install`, then
  `OUTPUT_DIR=artifacts uv run --frozen --project tools python deploy/vagrant/run_vagrant.py`.
- Vagrant provisioning uses `deploy/vagrant/provision.py` and shares
  `deploy/multipass/build_in_vm.py` with Multipass. To rebuild inside a provisioned guest, run
  `BUILD_DIR="$HOME/shadow_build" python3 /vagrant/deploy/multipass/build_in_vm.py --skip-dependencies`
  as the `vagrant` user. Qt versions and modules come from `.github/build-config.json`.

## Code Analysis

- `analysis.yml` runs on pull requests and `master` pushes over one path list (the
  `&analysis-paths` anchor: compiled `src/` files plus `analysis_matrix.FULL_SCAN_INPUTS`); there is
  no schedule. Manual dispatch runs one job for the selected tool (`clazy`, `clang-tidy`, `iwyu`,
  `qmllint`, or `tsan`).
- `analysis_matrix.py` emits each job's `family`, packages, CMake configure arguments, and
  `tools/analyze.py` arguments, so one Configure step serves every family. For a
  PR it estimates the compilation units from the diff and a textual reverse-include scan of `src/`
  and allots one shard per 50 units, up to four; it plans none when nothing compiled changed, and
  full-scan inputs keep all four. Each shard runs clang-tidy then Clazy over one selection and one
  `clang-scan-deps` scan, and reports errors independently. No shard saves the shared build
  caches: the first is the only one planned to (`save-cache`), but a shard builds only
  `qgc-analysis-headers`, and `cmake-build` saves them only after an `all` or `autogen` build.
  Upstream jobs use the 16-vCPU `linux-x64-analyzer` pool; forks use GitHub-hosted runners. The result cache that master pushes seed is described in
  [caching.md](caching.md#other-caches).
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
  clang-tidy runs only the `bugprone-*`, `clang-analyzer-*`, and `performance-*` checks, no style
  checks ([check policy](../../tools/analyzers/README.md#check-policy)), and reports headers only
  in the checkout's `src/`, `test/`, and `tools/`
  (`tools/tests/test_compiler_analysis.py::test_tidy_reports_only_this_checkout_s_headers`). On a
  pull request, the Build Results comment summarizes the warnings on changed lines and an inline
  review posts only the bug-class ones (errors, `bugprone-*`, `clang-analyzer-*`), at most 10
  comments per pull request ([reporting.md](reporting.md#build-results)). Jobs upload
  `*-timings.json`,
  `*-progress.jsonl`, `*-diagnostics.json` (each unique diagnostic's file, line, column,
  severity, and check, and on pull requests those on changed lines), and `*-raw.txt` (every unit's
  full output; identical diagnostic blocks are shown once in the log).
  Per-check profiling (`profile_checks` dispatch input) is described in
  [`tools/analyzers/README.md`](../../tools/analyzers/README.md).

## Lint, Docs, and Security Workflows

- **pre-commit**: `pre-commit.yml` has no path filter and runs `tools/pre_commit.py --ci --changed`
  (prek) on the pull request's, merge group's, or push's changed files, and uploads
  `pre-commit-results.json` and `pre-commit-output.txt` for the build-results comment. Pull request
  diffs come from `common.git.pr_diff_refs` (see [Change Detection](#change-detection)); pushes
  fetch full history to diff against `before`, and a base with no merge base in the fetched history
  checks everything. Qt is installed only when the selection includes QML for qmllint
  (`pre_commit.py --needs-qt`). The hooks, the manual-stage validators that `--ci` adds, and the
  failure hint are described in [tools/README.md](../../tools/README.md#pre_commitpy).
- **Whole-tree rechecks**: most hooks check only changed files, so a changed lint configuration or
  locked tool version (`ruff.toml`, `pyrightconfig.json`, `tools/pyproject.toml`, `tools/uv.lock`,
  the docs lint configuration) rechecks everything it governs; the hooks and their triggers are
  listed in [tools/README.md](../../tools/README.md#pre_commitpy). Renovate's lock updates, which
  touch no Python file, are therefore still linted.
- **Python and workflow lint** are pre-commit hooks, not a separate workflow: Ruff check and
  format, Pyright, the `tools/` import-linter layers (`ci` on top, `common` at the base),
  `tools/validate_configs.py`, and actionlint (which reads `.github/actionlint.yml`, listing the
  repository `vars`) with zizmor. Ruff, Pyright, clang-format, and import-linter run from the
  locked `lint` group, so the hooks and `tools/.venv` share one version.
- **Docs**: the Docs workflow checks English internal links without exemptions before building all
  locales. Its build sets `BRANCH_NAME` to the ref name, which `docs/.vitepress/config.mjs` uses as
  the site base (`/<branch>/`), matching the directory `deploy-docs` publishes to. VitePress is
  pinned to a 2.0 prerelease because the stable 1.x dependency chain prevents patched Vite updates;
  its native theme navigation replaces `vp-dynamic-nav`, and `npm run docs:build` runs its Node
  regression test first. Doxygen runs weekly; pull requests run it only when its configuration
  changes, because warnings do not fail the build.
- **Check Links** runs lychee on PRs, `master` pushes, and manual dispatch, and fails only pull
  requests; exact legacy URL exceptions in `tools/configs/lychee.toml` expire on their recorded
  review date (checked by `tools/validate_configs.py`).
- **Translations**: the `qt-translation-placeholders` hook checks new or changed TS messages
  against the PR merge's first parent or the push baseline (prek passes it as `PRE_COMMIT_FROM_REF`)
  without installing Qt. Unchanged legacy placeholder defects do not block unrelated imports;
  malformed XML and newly introduced placeholder mismatches fail. `ci-scripts.yml` runs the
  validator's tests.
- **Dependency Review** (`dependency-review.yml`) fails a pull request that adds a dependency with
  a high or critical advisory, and validates the Gradle wrapper.
- **CodeQL** scans Actions, Java/Kotlin, and Python (including `tools/`, `deploy/`, tests, and CI
  scripts); C++ is not scanned by CodeQL. It runs on `ubuntu-latest` with a 10-minute timeout: on
  `ubuntu-22.04` about one PR run in seven failed checkout and then hung in harden-runner's post
  step until the 30-minute timeout cancelled the check (counted for pull request 15370, October
  2026).
- **Bot pull requests**: GitHub starts no workflows for pull requests opened with `GITHUB_TOKEN`,
  so `lupdate.yml` and `px4-metadata.yml` (upstream only) open theirs with the
  `PX4BUILDBOT_ACCESSTOKEN` secret, which also opens the Crowdin pull requests. Without it they
  fall back to `GITHUB_TOKEN`, and the checks of those pull requests never report until someone
  pushes to the branch or closes and reopens the PR. Their titles and commit messages are
  Conventional Commits (`.github/workflows/crowdin.yml`, the root `crowdin.yml`), which the PR
  title check requires.

## Releases and Signing

The maintainer procedure (tag forms, what each publishes, verification, manual steps, hazards) is
the [Release Checklist](../../docs/en/qgc-dev-guide/release_branching_process.md). These are the
CI-side mechanics.

### Release tags

Platform workflows trigger on `push` of tags matching `v[0-9]+.[0-9]+.[0-9]+` only
(`test_workflow_triggers.py::test_release_tags_start_every_tag_workflow_through_one_filter`).
Release tags are cut on `Stable_V*` branches. Change detection (`_detect-changes.yml`, which also
plans the Android and Docker legs) skips every build for a tag ref that is not an exact `vX.Y.Z`
name reachable from an `origin/Stable*` branch (the name check also covers `workflow_dispatch` on a
tag, which bypasses the `push.tags` glob), so nothing is built, signed, or pushed (TestFlight, Play
Store, Docker Hub). `aws_upload.py promote` independently refuses to upload such a tag to the S3
`latest/` folder and fails its job, so the step after it, which invalidates CloudFront, does not
run. Both use
`qgc_tools.release_tag_check.check_release_tag()`, which fails the job if git cannot answer.
Version-marker tags on `master` (e.g. `v5.2.0-dev`, consumed by `git describe` for daily-build
version numbers) do not match the trigger glob and never start a build.

- The push of a release tag starts a staged Play Store production rollout at 20% (master pushes
  complete a beta release), uploads the App Store build to TestFlight, and pushes the builder
  images to Docker Hub. The Play Store and TestFlight steps run only when their secrets and
  variables are set; otherwise they are skipped without failing the run (TestFlight with a
  notice). The
  packages reach S3 `builds/<tag>/` and `latest/`, and the tag's draft GitHub Release, through
  [Publish Builds](#publish-builds), once Linux, Windows, MacOS, and Android have all passed
  for the tag; `latest/` keeps a release that is newer than the tag.
- **Only a push uploads to a store.** The Play Store step of `android.yml` and the TestFlight
  step of `ios.yml` name `github.event_name == 'push'` among their conditions, so a run that was
  started by hand, by a pull request, or by the merge queue uploads to neither, whatever ref it
  builds. `test_workflow_expressions.py::test_a_build_is_uploaded_to_an_app_store_only_by_a_push`
  finds such steps by their upload actions, by the store tools that a script would call, and by
  the Play Store service account, and fails when one lacks the condition. A platform workflow
  dispatched on a release tag builds what the tag's push builds and uploads it to no store:
  `android.yml` the APK signed with the release keystore, and `ios.yml` the IPA signed for the
  App Store, with a notice that it stays a workflow artifact. Both packages are attested and
  kept as workflow artifacts, and Publish Builds, which counts `push` runs only, publishes no
  download from them. `android.yml` dispatched on `master` builds the APK and leaves the beta
  track alone; the App Bundle was a `master` push's alone before. To repeat a store upload that
  failed, re-run the failed job of the push run: a re-run keeps the event of its run.
- Release builds use no compiler or moc cache ([caching.md](caching.md#release-tags)).
- `devcontainer.yml` publishes `ghcr.io/mavlink/qgc-dev:vX.Y.Z`. It does not call
  `_detect-changes.yml`: its first job, which the build needs, runs
  `tools/ci/publish/release_branch_check.py` on a tag push and fails unless an `origin/Stable*`
  branch contains the tag, so no image is built or published for a tag anywhere else. Every
  workflow a release tag starts makes one of the two checks in a job its other jobs wait for
  (`test_workflow_triggers.py::test_release_tag_workflows_check_the_stable_branch_before_their_other_jobs`).
- A re-pushed tag queues a second run instead of cancelling the first: the pushes of a tag share
  a per-ref concurrency group and are never cancelled, so the stores and registries are published
  to again when the first run finishes. Publish Builds then finds that S3 already holds the
  release and uploads nothing there. Its GitHub Release job gives a draft the rebuilt packages,
  and fails for a published release, whose files differ from them, until a dispatch with
  `force_release` replaces them, which nothing can do for an
  [immutable](#release-immutability) release. A run started by hand has a group of its own, so
  it does not replace the push run that is waiting for its turn
  (`test_workflow_expressions.py::test_a_run_started_by_hand_never_displaces_a_push_run_that_is_waited_for`).
- The tag push also runs `release.yml`, which stops with an error unless an `origin/Stable*`
  branch contains the tag (`tools/ci/publish/release_branch_check.py`), so no release is drafted
  for a tag that is never built. It then drafts the tag's GitHub Release with git-cliff notes
  (`release_notes.py`, `tools/configs/cliff.toml`; `just release-notes vX.Y.Z` prints them locally)
  and writes them to the job summary. A tag that already has a release, draft or published, keeps
  it unchanged; a manual dispatch with the `tag` input drafts an older tag with the dispatching
  branch's tooling. Publish Builds attaches the packages to the draft; publishing it is a
  maintainer's step, to be taken after that: the job fails for a release that was published
  without its packages.

### Publish Builds

`publish.yml` publishes the packages of `master`, `Stable*` branches, and release tags. The build
workflows and the actions they use hold no AWS credentials
(`test_workflow_security_policy.py::test_only_publishing_and_infrastructure_workflows_hold_aws_credentials`).
It starts when Linux, Windows, MacOS, or Android completes (the workflows that mark a package for
publishing,
`test_published_builds.py::test_publish_follows_and_requires_exactly_the_workflows_that_mark_packages`)
and by `workflow_dispatch` with a `ref`, an optional `commit` of it, and an optional `dry_run`.
Every pull request's builds
complete too: the `decide` job's `if:` skips a build that is not a `push` build of this
repository that concluded `success` or `failure`, so such a run ends with every job skipped and
uses no runner (`test_publish_workflow.py::test_a_build_that_cannot_publish_starts_no_job`). The trigger has no
`branches` filter, which would avoid even that run: the documentation says that the filter
names "what branches the triggering workflow must run on" and nothing of a build that a tag
started, and a filter that left tags out would stop every release. On `mavlink/qgroundcontrol`
it publishes without any repository variable, from the first build that completes on `master`
([Going live](#going-live),
`test_publish_workflow.py::test_upstream_publishes_for_real_with_no_repository_variable_set`).

- **Marker**: `attest-and-upload`'s `publish` input makes `attest_helper.py prepare` add
  `"publish": {"name": "<file>", "workflow": "<workflow>", "packages": [...]}` to the package's
  `<file>.build.json` in the workflow artifact, beside the repository, ref, commit, event,
  workflow, run, attempt, and job that file records. `packages` is every name that
  `PUBLISHED_BUILDS` gives the workflow in the commit that is built, and the step fails for a
  package that is not among them. On `master` those are six packages. `job_id` is
  `job.check_run_id`, the id that the jobs API gives the job (`job`, from `GITHUB_JOB`, is only
  its key in the workflow). Each step that can mark a package passes it as the action's
  `job-id` input, because the `job` context is documented for a workflow's steps, not for a
  composite action's
  (`test_published_builds.py::test_every_step_that_can_mark_a_package_passes_its_job_s_id`).
  The runners gave every such job its id in pull request 15452's runs: RunsOn Linux and Windows,
  `windows-11-arm`, `macos-15`, and Android. A runner that gives the job no id does not fail the
  build: the step warns, marks the package without an id, and `publish_set.py` refuses it, so
  the failure is a Publish Builds run's. The build provenance attestation covers the package,
  this file, which no later step changes, and the package's companion file when it has one
  (_AppImage update files_, below)
  (`test_published_builds.py::test_the_build_attests_the_metadata_that_marks_a_package_and_names_its_job`).
  A run without markers (a branch whose own workflows still upload to S3, such as
  `Stable_V5.1`) publishes nothing here.
- **Decide** (`publish_decide.py decide`; read-only token, no secrets): the triggering run must be
  a `push` run of one of the four workflow files, in this repository, on `master`, a `Stable*`
  branch, or a `vX.Y.Z` name. The name is not trusted, because `workflow_run.head_branch` holds a
  tag's name for a tag push: a branch must contain the commit, and a tag must exist, be that
  commit, and be on a `Stable*` branch. Every workflow's most recent `push` run for the commit
  must have completed with a conclusion that its ref is published from (next item), so the run
  that completes the set publishes and the earlier ones end with "waiting for …" in their
  summary. The runs are looked up by commit, however many pushes followed it. A re-run counts by
  its latest attempt: the event of an earlier attempt does not stand for a later one that is
  running or was stopped. A dispatch publishes the newest commit of the ref whose builds have all
  finished, reading as many pages of runs as that takes. That commit may lack a package, because
  a job that builds one failed, and the publish then ends with "nothing to publish yet". The
  `commit` input reaches an older commit of the branch that can be published: it takes all 40
  digits of the commit's SHA, in lower case, and the script refuses any other value before it
  asks GitHub anything. The commit is decided for like any other: every platform's run of it
  must have finished, it gives way to a newer commit that passed on every platform, and no
  target goes back to it. A tag names one commit, so for a tag the input can only repeat it.
  Only `push` runs count: a platform
  workflow dispatched by hand publishes no download, and uploads to no store
  ([Release tags](#release-tags))
  (`tools/tests/ci/test_publish_decide.py::test_a_manual_request_can_name_an_older_commit_than_the_newest_finished_one`,
  `tools/tests/ci/test_publish_decide.py::test_a_commit_named_by_hand_never_takes_a_target_back`,
  `test_publish_workflow.py::test_what_a_request_names_reaches_the_decision_through_the_environment`).
- **Which conclusions publish** (`publish_set.PUBLISHABLE_CONCLUSIONS`): a release tag needs
  `success` from all four workflows. A branch (`master`, `Stable*`) also takes `failure`, so
  that a job which builds no package, such as the Android emulator boot test or a unit-test leg,
  does not hold back every platform's packages. The decision then approves the commit, names
  each run's conclusion, and leaves the question that it cannot answer to verification, which
  reads the artifacts: whether a job that passed built every package (_A failed build is not a
  failed publish_, below). Every other conclusion waits for a re-run:
  `cancelled`, `timed_out`, `skipped`, `neutral`, `action_required`, `stale`, and
  `startup_failure` say that the run was stopped or never ran, and nothing of its commit. GitHub
  also reports a job that it stops for running out of time as `cancelled`, and the run with it
  (upstream runs 42306314879 and 42306314813, whose jobs waited 24 hours for a runner), so such
  a commit is published only after the job is re-run. A commit gives way only to a newer one
  that passed on every platform and that its branch still contains, which is sure to publish
  itself. A newer one with a failed workflow may lack a package, so it stands in the way only
  once a target holds it. When a failed job is re-run later and passes, nothing is published
  again: the targets hold the commit, and what they recorded of its runs stays as it was at
  publication, with the attempt. A dispatch publishes the commit again and brings that up to
  date
  (`tools/tests/ci/test_publish_decide.py::test_a_branch_commit_is_published_although_a_workflow_failed`,
  `tools/tests/ci/test_publish_decide.py::test_a_release_tag_waits_for_every_workflow_to_pass`,
  `tools/tests/ci/test_publish_decide.py::test_a_failed_job_that_is_re_run_and_passes_does_not_publish_the_commit_again`,
  `tools/tests/ci/test_publish_decide.py::test_a_commit_does_not_give_way_to_a_newer_one_whose_packages_may_be_missing`).
- **Never regress**: each target records what it holds: the daily release its commit, in its
  notes, and an S3 directory its ref and commit, in `published.json`, which is written last. A
  branch's commit is published to a target only when it is ahead of that commit (GitHub's compare
  API). A commit the target already holds is published again only by a dispatch, which is how
  packages rebuilt for the same commit (a re-run build) get published. Each publishing job
  repeats the check when it gets its turn (`publish_decide.py confirm`), and `aws_upload.py`
  makes it once more on the record as it reads it before its first upload: over the record's
  public URL by default, which costs the role no permission and cannot tell a record that is
  not public from none (_S3 records_, next). For a tag, `confirm` and the scripts that write
  (`aws_upload.py publish` and `promote`, `release_assets.py`) resolve the tag again,
  following an annotated tag to its commit, and publish nothing once it was moved or deleted
  (`publish_set.tag_problem`). A target whose commit is not an ancestor (a rewritten `Stable*`
  branch) fails the run until someone deletes its record.
- **S3 records**: every reader takes `published.json` from the bucket's public endpoint (not
  CloudFront, whose copy can be stale): `decide` and `confirm`, to see whether a job starts,
  and by default `aws_upload.py` itself, before its first upload (`--record-read public`). The
  record is uploaded `public-read` like the packages, so the read is an anonymous HTTPS GET.
  The role needs no read permission for it, and keeps the permissions that the build jobs'
  uploads always used. The bucket cannot be listed, so a missing object answers 403 there (404
  if it could be), and either means that nothing is published, for `latest/` not even that
  (below). Any other answer stops the job with nothing uploaded: a 5xx, no connection after
  the retries, a body that is not a record. So does a record that is newer than the set.
  - **The price of the public read**: a record that exists and is not public answers 403 as
    well, and passes for none. `latest/` then refuses every release. A `builds/<ref>/`
    directory is compared with nothing, so an older commit would replace a newer one there.
    The read-back is what catches it: after each upload the job fetches its own record from
    the same URL, and fails when it cannot, with an error that says to make the object public
    before anything else is published. A record therefore cannot stay unreadable unnoticed,
    but the upload that finds it out has happened by then.
  - **The credentialed read** (`--record-read credentialed`) reads with
    `aws s3api get-object` instead. Only "no such key" then means that nothing is published,
    and access denied stops the job before it uploads, so a record that is not public is
    still compared with. To switch to it, add `PUBLISH_RECORD_READ: credentialed` to the
    `env` of the `s3-builds` and `s3-latest` jobs, where `publish.yml` has a comment for it.
    Grant the role the two permissions that only this read takes first: `s3:GetObject` on the
    records and `s3:ListBucket` on the bucket ([AWS setup](#aws-setup)). No repository
    variable selects the read, and a value other than the two fails the job.
  - **Dry runs** start no `aws` command with either read, and configure no credentials. What
    they read of S3 is the public record, in `decide` and `confirm`, which is an HTTPS GET
    and no AWS API call.

  After the upload, the record must be readable from the public endpoint with either read,
  as the packages are. A record names its repository, and one that another repository wrote
  is not compared with: its commits mean nothing here. The decision reports it and starts the
  job, so that a dry run in a fork, which reads upstream's bucket, does not fail over a branch
  that "was rewritten", and `aws_upload.py` refuses to replace it. The record also lists the
  workflow runs that built the files (`builds`: workflow, run, attempt, and conclusion at
  publication)
  (`tools/tests/ci/test_aws_upload.py::test_a_public_record_that_cannot_be_read_stops_the_job_before_it_uploads_anything`,
  `tools/tests/ci/test_aws_upload.py::test_a_record_that_the_public_endpoint_hides_is_found_by_the_read_back_of_the_next_write`,
  `tools/tests/ci/test_aws_upload.py::test_a_dry_run_uploads_nothing_and_makes_no_call_that_needs_credentials`,
  `test_publish_workflow.py::test_the_record_is_read_from_its_public_url_which_needs_no_permission_of_the_role`).
- **`latest/`** is ordered by release version, not by commit. `v5.1.6` replaces `v5.1.5`, also
  when both tag one commit. A tag older than the release `latest/` holds (a late run, or a patch
  release on an older Stable line) is published to its `builds/<tag>/` only. The release it
  holds is replaced by the same release at another commit: a tag that was moved forward and
  built again reaches `latest/` as it reaches `builds/<tag>/` and the draft
  (`tools/tests/ci/test_publish_decide.py::test_a_release_tag_moved_to_another_commit_replaces_what_latest_holds_of_it`).
- **`latest/` without a record** is not taken for empty: it held a release before this workflow
  recorded any, and that release may be newer than the tag. The tag is published to
  `builds/<tag>/`, and the `S3 latest/` job then fails with nothing uploaded, naming the two
  ways on: [seed the record](#seeding-the-latest-record) and re-run the job, or dispatch with
  `force_latest`. The decision reads the public endpoint, where a missing record and a private
  one look alike, so it starts the job instead of skipping `latest/`; the job reads the record
  again, by either read, and refuses without one. A dry run reports this for the `latest`
  target and starts no `aws` command.
- **`force_latest`** (a dispatch input) replaces whatever `latest/` holds: a newer release (a
  rollback), a release without a record, or a record that names no release tag, which stops
  the decision otherwise. It counts only in a run dispatched from `master`: a completed build
  has no inputs, and a dispatch from another ref is a dry run
  (`test_publish_workflow.py::test_only_a_run_started_by_hand_on_the_default_branch_can_force_a_target`).
  `force_release`, under the same rule, is the only way to change a GitHub Release that is
  published, and no way to change one that GitHub reports as immutable
  ([Release immutability](#release-immutability)). Neither input does the other's work.
- **Fetch and verify** (`publish_set.py fetch`; read access to contents, actions, and
  attestations): the packages are downloaded from exactly those runs by artifact id, the newest
  artifact of each name, since a re-run adds a second one. Each run is approved at one attempt,
  with the conclusion it had. A package built by a later attempt fails the set, and when a run's
  latest attempt is no longer that one, before the downloads or after them, the job warns and
  publishes nothing: a re-run that completes starts a publish of its own. Each marker must agree
  with its run, its
  name must be one that `master` lists (`build_artifacts.is_publishable_name`), the file must
  match its `.sha256`, and `gh attestation verify` must find a build provenance attestation
  signed by that workflow file for the ref and commit (`--signer-workflow`, `--source-ref`,
  `--source-digest`, `--signer-digest`), for the package and for the `<file>.build.json` that
  marks it. A marker rewritten after the build, for instance to list fewer packages, has none.
  Self-hosted runners are allowed, because upstream builds on RunsOn. The verified files become
  the `publish-set` artifact, the only input of the publishing jobs, which hash them again
  (`load_set`). Its manifest names the four runs with their attempts and conclusions, which
  the job summaries, the daily release's notes, and `published.json` repeat.
- **A failed build is not a failed publish**: when a run that concluded `failure` lacks a package that its markers list, has a
  package whose job did not pass, or has no marked package while another workflow has, the
  build failed where it matters and nothing is wrong with publishing. `fetch` then ends with
  `publish=false`, a notice, and a summary that reads "nothing to publish yet" with the package
  and the run; the Publish Builds run stays green and no publishing job starts. The same from a
  run that concluded `success` fails the set, as it always did. The runs that failed are read
  first, so the gap is found before the other platforms' packages are downloaded. Whatever else
  is wrong with a failed run's package (its attestation, its checksum, its marker) fails the
  set like any other's
  (`tools/tests/ci/test_publish_set.py::test_a_failed_run_that_lacks_a_package_ends_quietly_with_nothing_to_publish`,
  `tools/tests/ci/test_publish_set.py::test_a_failed_build_is_no_excuse_for_a_package_that_does_not_verify`).
- **Expired artifacts**: `attest-and-upload` keeps a push build's artifact for 30 days
  (`ARTIFACT_RETENTION_DAYS`), and GitHub then leaves it out of the run's listing: upstream's
  Linux run 36878132470 for `v5.1.5`, of 2026-10-01, listed `size-metrics` alone on 2026-11-12.
  A run that passed and lists no package artifact therefore fails the set, whether a completed
  build or a dispatch asked. For a branch the error says to re-run all jobs of the run. For a
  release tag it says not to: the tag's push runs would upload to the stores and push the
  builder images again, and those of a branch that uploads for itself (`Stable_V5.1`) would
  write `latest/`. It names `builds/<tag>/` on the download server instead, and the passages
  of the Release Checklist that publish from there by hand. Such a run is not taken for a build
  that publishes for itself, which lists its packages without markers
  (`tools/tests/ci/test_publish_set.py::test_runs_whose_artifacts_expired_fail_the_request_instead_of_passing_for_an_old_layout`).
- **AppImage update files**: upstream's AppImages embed the update information `gh-releases-zsync|mavlink|qgroundcontrol|latest|<name>.AppImage.zsync`
  (`linux.yml`'s `QGC_APPIMAGE_UPDATE_INFORMATION`: every event and ref of
  `mavlink/qgroundcontrol`, and nothing in a fork), which sends an updater to the repository's
  latest GitHub Release for that file. appimagetool writes it beside the AppImage: a text header
  (`zsync`, `Filename`, `MTime`, `Blocksize`, `Length`, `Hash-Lengths`, `URL`, `SHA-1`), a blank
  line, and the block checksums (347,453 bytes for the 203 MB AppImage of upstream's `master`
  run 42969994287). The AppImage's step passes the file as `attest-and-upload`'s
  `companion-path`, which attests it with the package, uploads it in the artifact, and lists it
  with its SHA-256 in the publish marker (`"companions"`). A companion is no package: no
  `packages` list names it, and the only name ever accepted is an AppImage's own with `.zsync`
  (`build_artifacts.companion_names`). When a marker lists one, `publish_set.py` requires it in
  the artifact, with that SHA-256 and the build's attestation, and requires its header to
  describe the verified AppImage: `Filename` and `URL` are the package's name (an updater
  downloads what `URL` names, relative to the `.zsync`), `Length` is its size, `SHA-1` its
  digest, and no `Z-URL` offers another download. Any difference fails the set. A build without
  update information lists none and is published as before. The file goes to the tag's GitHub
  Release only, where the update information points; S3 and the daily release do not get it.
  `master` and `Stable*` branch builds embed the same information, so an updater run on a daily
  build fetches the latest stable release
  (`test_published_builds.py::test_an_appimage_that_embeds_update_information_is_uploaded_with_its_update_file`,
  `tools/tests/ci/test_publish_set.py::test_an_update_file_must_describe_the_verified_package`).
- **The job behind a package**: the attested metadata names the job and the attempt that built
  the package; a marked package that names no job fails the set. That job must have concluded
  `success` and belong to the approved attempt
  (`runs/<id>/attempts/<n>/jobs`). A re-run of the failed jobs lists every job it did not run
  again as a copy: a new id, the name and the start and completion times of the original, and
  nothing else that links the two (upstream run 42969994287 shows it). A package of an earlier
  attempt therefore counts only when its job passed in that attempt, no other job of that
  attempt has its name and times, and exactly one passed job of the approved attempt has them.
  When several jobs share them, the job that built the package cannot be identified and the
  set fails, naming the job and the run. Any other comes from a job that failed after its
  upload or that was run again, so the approved attempt's artifact of that name is missing,
  and the older package never takes its place. What follows depends on the run, as for any
  package that a run lacks (_A failed build is not a failed publish_, above): for a run that
  concluded `success` the set fails and names the artifact, and for a branch's run that
  concluded `failure` the publish ends with "nothing to publish yet". Re-running the job
  builds the package again, and the completion of that re-run starts a publish of its own.
- **A complete set** is judged by the build, not by `master`. The packages of a workflow's run
  must all carry the same `packages` list and be exactly the packages on it, and every one of
  the four workflows must provide at least one. A missing package fails a run that passed,
  because it would drop out of the daily release and keep its previous file on S3 under a record
  that names the new commit, and ends the publish of a run that failed with nothing published;
  a marked package that the list lacks fails either. A `Stable*` branch cut before
  `master` gained a package lists fewer and is published with those. `master` decides only
  which names may be published at all: a name it does not list fails, whatever a build says.
- **Trust**: a `workflow_run` run uses the default branch's copy of the workflow and of `tools/`,
  never the triggering run's code
  (`test_workflow_security_policy.py::test_privileged_workflows_check_out_only_their_own_ref`).
  Artifacts are data: nothing in them is executed, and only allow-listed names become a path or
  a key. A run dispatched from another branch is always a dry run.
- **What deleting artifacts can do**: an artifact without a marker is skipped, not failed,
  because the packages that are not published and the branches that publish for themselves
  have none. Whoever can delete a run's artifacts can therefore stop a publish, but not
  shrink one. A package that remains lists the others of its workflow. A run that passed and
  has no marked package left fails the set, as expired when it lists no package artifact at
  all; a run that failed ends the publish with nothing published; and when no run has a marked
  package left while some still list packages, the commit is taken for one that publishes for
  itself: nothing is published.
- **What the checks do not prove** is that a `Stable*` branch and its tag are meant to be a
  release. Anyone with write access can push a new or unprotected `Stable*` branch that carries
  its own workflow files, and a fresh `vX.Y.Z` tag on it. Its packages are built and attested by
  this repository's workflows like any others, so the tag is published to `builds/<tag>/` and,
  when its version is the highest, to `latest/`. This is unchanged from when the build jobs
  uploaded for themselves. Only rulesets that restrict who creates `Stable*` branches and `v*`
  tags limit it, and they are repository settings, not files here. Upstream has none today
  (read on 2026-11-12): its _Tag Protection_ ruleset blocks the deletion and the
  non-fast-forward update of a tag and nothing else, and no ruleset names `Stable*`. The
  approval gate below is not in place either, and would cover less: it holds back `latest/`
  alone, not `builds/<tag>/`, and not the Play Store rollout, TestFlight, Docker Hub, or GHCR,
  which the tag's own workflows publish.
- **Daily release** (`daily_release.py`; `contents: write`; `master` only): one prerelease on the
  `daily` tag, never the latest release, which the README's buttons download, and updated in place
  so that watchers are notified once. The new files are uploaded as `<name>.next`; the tag moves
  with an unforced ref update, which GitHub and the tag ruleset accept only as a fast-forward;
  each old asset is then deleted and its `.next` renamed, which takes seconds; the notes, which
  record the commit, are written last. That record, not the tag, keeps an older commit out. The
  first publication creates the tag and a draft, and the change that writes the notes also
  publishes the draft, so the release appears with its files. A first run that failed leaves
  the draft, which the next run finds in the listing of releases and finishes. The release
  needs the repository's release immutability setting to be off, and the job refuses a
  release that GitHub reports as immutable ([Release immutability](#release-immutability))
  (`tools/tests/ci/test_daily_release.py::test_the_first_publish_is_a_draft_until_its_files_and_notes_are_complete`,
  `tools/tests/ci/test_daily_release.py::test_a_rerun_publishes_the_draft_that_a_failed_first_publish_left`).
  The
  notes also name each workflow's run and say how it concluded: every file was built by a job
  that passed, and a run that did not pass as a whole is named as such, since `master` is
  published although a job that builds no package failed
  (`tools/tests/ci/test_daily_release.py::test_the_notes_do_not_pass_a_build_for_one_whose_workflows_all_passed`).
  When
  GitHub refuses to move the tag (HTTP 403, 409, or 422), the job warns and goes on, so that a
  tag that cannot move does not stop every later publish: the files are replaced, the notes name
  the commit they were built from and say where the tag still points, and the next publish
  tries the move again. Any other failure of the move, and a tag that cannot be created for the
  first release, fails the job before a file is replaced. `cmake/modules/Git.cmake` describes
  version tags only (`--match`), so the `daily` tag changes no version (CTest's
  `CMake.GitConfigureDepends`). Not yet exercised on GitHub: the move of the `daily` tag under
  upstream's _Tag Protection_ ruleset, and with it the path taken when GitHub refuses the move,
  have run against the tests' stand-in for the API only.
- **The `daily` tag in a clone**: the tag moves with every daily build, and `git fetch --tags`
  refuses to move a tag that the clone already has (`! [rejected] daily -> daily (would clobber
  existing tag)`), while fetching everything else. `git fetch --tags --force` takes the new
  one, and so does deleting the local tag first (`git tag -d daily`). A plain `git fetch` or
  `git pull` says nothing and leaves the local tag where it was first fetched (git 2.43), so a
  clone's `daily` is not where the release is unless it was forced. Upstream, the tag
  can be neither deleted nor moved backwards (_Tag Protection_ allows neither and lists no bypass
  actor). The daily release does not need it to: what the release holds is the commit in its
  notes. If the tag must be reset all the same, an administrator has to disable that ruleset or
  add a bypass for the moment, delete the `daily` release and tag, and dispatch
  `publish.yml -f ref=master`, which creates both again for the newest finished commit. Check
  first that release immutability is off: with it on, the new release is locked when it is
  published, and the tag name of a deleted release that was immutable cannot be used again
  ([Release immutability](#release-immutability)).
- **S3** (`aws_upload.py`; `id-token: write`; the `mavlink` owner only): `publish` uploads the set
  to `builds/<ref>/`; `promote`, in its own job and for release tags only, then uploads it to
  `latest/`. Each writes the record last. The keys are the ones the build jobs used to write.
  Without AWS credentials both warn and upload nothing. A failed `aws` command ends `publish`,
  `promote`, and `invalidate` alike with an error annotation and a failed job, never with a
  traceback
  (`tools/tests/ci/test_aws_upload.py::test_whichever_aws_call_fails_the_command_reports_an_error_and_fails`).
- **CloudFront**: the download links go through CloudFront, the user guide's to
  `builds/master/` and the stable ones to `latest/`, and it would serve its cached copy of a
  file, or a new package beside an old `.sha256`. After `publish` or `promote`, the job's next
  step (`aws_upload.py invalidate`, with `--latest` in _S3 latest/_) sends one wildcard path,
  `/builds/<ref>/*` or `/latest/*`. CloudFront accepts a wildcard only at the end of a path and
  counts such a path as one, whatever it covers. The step also runs when the directory already
  held the commit or the release and nothing was uploaded: after a failed invalidation the
  files and the record are in S3 and correct, the job fails with an error that says so, and
  _Re-run failed jobs_ sends the invalidation alone, since the job's `confirm` step finds the
  directory up to date and skips the upload. A run that was started by hand publishes again
  whatever the directory holds, so its re-run uploads the files again first, as a new dispatch
  for the ref does. Nothing else remembers a failed invalidation; the next publish of the
  directory covers the same path. A dry run sends none, and without `AWS_DISTRIBUTION_ID` the
  step warns once.
  Expect about 100 paths a month for `master` (one per published commit; 105 commits in the 30
  days to 2026-11-12), one per `Stable*` push and per tag, and one more per release for
  `latest/`. The first 1,000 paths a month are free, counted over the whole AWS account
  (`test_publish_workflow.py::test_a_directory_is_invalidated_after_its_upload_and_also_when_that_had_nothing_to_do`,
  `tools/tests/ci/test_aws_upload.py::test_a_failed_invalidation_fails_the_job_and_leaves_the_record_as_uploaded`).
- **AWS**: the role and its trust policy, each permission with the call that needs it, the
  check to make before the merge, and what the jobs print when something is missing are under
  [AWS setup](#aws-setup).
- **GitHub Release assets** (`release_assets.py`; `contents: write`; release tags only): the
  set's packages, their `.sha256` files, and each AppImage's `.zsync` go to the tag's release
  under the same names. The README's buttons download the packages from the latest release,
  and an AppImage's updater looks for its `.zsync` there. Only a draft is changed. A published
  release is only checked, unless a dispatch sets `force_release`: the job passes when the
  release has every file of the set with the same SHA-256, and otherwise fails, naming the
  files that the release lacks or holds with other content and the dispatch that attaches
  them. A release that GitHub reports as immutable is only ever checked, with `force_release`
  too: nothing can change its files, and the error says that instead of naming a dispatch
  ([Release immutability](#release-immutability)). A release that was published before its
  packages arrived is thus a failed job, not a
  release whose download buttons answer 404 and that nobody notices. One change is made to a
  published release without `force_release`: `<name>.next` uploads that a failed run of this
  job left are removed, because they are no files of the release and its users see them. They
  carry the set's own names, and the job's concurrency group keeps another run from uploading
  meanwhile. A tag without a release
  fails the job after a wait of a few minutes: `release.yml` creates the release, this job
  never does, and its message names both workflows. `releases/tags/<tag>` answers for
  published releases only and a listing shows drafts only to a token that may write, so the
  decision always starts the job, and the job finds the release, draft or published, in the
  listing and uses its id. Two releases for one tag stop it. It writes assets only, never
  the notes, the title, or the draft, prerelease, and latest settings. An asset that already
  has the file's SHA-256 (the API's `digest`) is left alone, so a second run changes nothing;
  the others are uploaded as `<name>.next` and swapped in by `daily_release.py`'s helpers, and
  an interrupted run leaves the old asset or a `.next` one that the next run deletes. Only the
  `.next` names of the set's own files are ever deleted; every other asset stays, whatever its
  name ends in. The tag is resolved again before the uploads and before the swap, and the
  release is read again by its id before every change. A draft that was published meanwhile is
  changed no further: the run withdraws its own `.next` uploads and fails, naming the files
  that are missing and the `force_release` dispatch that attaches them. When the release is
  immutable by then, the run leaves the uploads, which GitHub would not let it delete, and
  says that the release keeps what it has. A release published in
  the moment between a check and the change that follows it still gets that one change. It has
  no record: a draft takes rebuilt packages of the same commit from any run, while S3 takes
  them only by dispatch. A build that is re-run after its release was published therefore
  fails this job, since the release's files differ from the rebuilt ones, and leaves the
  release as it is. The job does not depend on the S3 jobs
  (`test_publish_workflow.py::test_a_tag_s_packages_reach_its_github_release_in_a_job_of_their_own`,
  `tools/tests/ci/test_release_assets.py::test_a_release_published_before_its_packages_were_attached_fails_the_job`,
  `tools/tests/ci/test_release_assets.py::test_an_immutable_release_that_lacks_files_fails_and_nothing_can_attach_them`).
- **What a publication holds**: each target gets exactly the verified set, whatever packages it
  has, and the tag's GitHub Release the AppImage update files besides. The daily release drops
  every asset that is not part of it. S3 never deletes: a file
  under a name that the set lacks, such as a package that a later release line no longer builds,
  stays in `builds/<ref>/` or `latest/` and stays downloadable. `published.json` and the job
  summary list the files that belong to the publication, and the job warns of the names that
  the previous record lists and the set lacks. Delete such a file by hand, with its `.sha256`.
- **Concurrency**: the workflow has no group of its own: most runs find nothing to publish and
  have nothing to wait for. Each publishing job is in its target's group
  (`publish-daily-release`, `publish-s3-<ref>`, `publish-s3-latest`, `publish-release-<ref>`),
  is never cancelled, and
  queues (`queue: max`). A group otherwise keeps one waiting job, so a later job for an older
  commit would replace the waiting publish of the newest one
  (`test_workflow_expressions.py::test_jobs_another_workflow_starts_with_write_access_take_turns_without_being_replaced`).
  GitHub starts the waiting jobs roughly in the order they began to wait, which `confirm` makes
  harmless, and cancels a job that would be the 101st in line: publish its commit by dispatch.
- **Operating it**: runs are listed under `master` and named "_workflow_ completed for _ref_" or
  "Publish _ref_", with "at _commit_" when the dispatch names one. A failed publish fails this
  workflow, not a build, and a failed build does
  not fail this workflow: a green run whose _Fetch and verify_ summary reads "nothing to
  publish yet" names the package that a failed build did not produce, and re-running that
  build's failed jobs publishes the commit. Its jobs fail
  independently, and _Re-run failed jobs_ repeats only those, for the three days that the
  `publish-set` artifact is kept. To publish by hand:
  `gh workflow run publish.yml -f ref=<branch or tag>`, with `-f commit=<full SHA>` to publish
  that commit of a branch instead of the newest finished one, with `-f dry_run=true` to only
  report, with `-f force_latest=true` to let a release tag replace whatever `latest/` holds, and
  with `-f force_release=true` to replace the files of its published GitHub Release, unless
  that is [immutable](#release-immutability).
- **Approval gate** (not enabled): `environment: release` on the `s3-latest` job would require an
  approval before `latest/` changes, and before nothing else. The job has no `environment:` line
  today, so creating the environment alone gates nothing. Adding the line also changes that
  job's OIDC subject to `repo:mavlink/qgroundcontrol:environment:release`, so the environment
  must exist and the AWS role's trust policy must allow that subject first
  ([AWS setup](#aws-setup)).
- **Forks** run none of this, except a dispatched dry run, unless the repository variable
  `QGC_PUBLISH_DAILY_RELEASE` is `true`, which gives the fork a daily release of its own and
  attaches a release tag's packages to the fork's draft release. Such a fork needs
  [release immutability](#release-immutability) off, as upstream does. S3 is upstream's alone,
  and a
  fork's dry run reports the bucket's records as another repository's instead of comparing its
  commits with them.

#### Going live

Nothing switches Publish Builds on. On `mavlink/qgroundcontrol` it publishes from the moment it
is on `master`: the first publication is that of the first `master` commit after the merge
whose Linux, Windows, MacOS, and Android runs have all finished, normally the merge commit. The
same change stops the build workflows on `master` from uploading for themselves, so until that
run `builds/master/`, which the user guide links, keeps the last build that was uploaded before
the merge. Older branches upload for themselves as before. Upstream had neither a `daily` tag
nor a `daily` release, and no `published.json` in `builds/master/` or `latest/`, on 2026-11-12.

**Before the merge**, an administrator checks that release immutability is off:
`gh api repos/mavlink/qgroundcontrol/immutable-releases` must print `"enabled":false`, as it
did on 2026-11-12. With it on, the first daily release is locked when it is published: no
later build can update it, and the tag name `daily` is used up
([Release immutability](#release-immutability)).

Nothing has to change in AWS. The role uploads and invalidates as the
build jobs did with it, and the record is read over its public URL, which takes no permission.
Make the two anonymous checks under [AWS setup](#aws-setup). A dry run asks for no
credentials, so the first publication is the first run that uses the role from this workflow.

**Before the first release tag** that this workflow publishes (a tag on a branch cut from
`master` after the merge), `latest/published.json` must be
[seeded](#seeding-the-latest-record). `master` never reaches `latest/`, so the merge does not
wait for the seed.

**What the first run does.** Every build that completes starts a run. The three that the
commit's first builds start end with "waiting for …". The one that its last build starts
decides _✅ publish_ with the targets `daily` and `s3`, both "nothing is published yet", and
_Fetch and verify_ verifies six packages. _GitHub daily release_ then creates the `daily` tag
at the commit and a draft, uploads the six packages and their six checksums to it, and
publishes it with its notes as the prerelease _QGroundControl Daily Build_. _S3 builds/_ reads
`builds/master/published.json`, which does not exist yet,
uploads the same twelve files and then the record, reads the record back from its public URL,
and invalidates `/builds/master/*` in CloudFront. The two jobs do not depend on each other:
when one fails, the other's publication stands. The `daily` tag has not been created or moved
under upstream's _Tag Protection_ ruleset yet (_Daily release_ above), so the first runs are
the first that show it.

**Check it**:

- `gh run list --workflow publish.yml --limit 10` shows the run named "_workflow_ completed
  for master" after the last of the four builds. It is green, and the summaries of its two
  publishing jobs read "✅ published".
- `gh release view daily --repo mavlink/qgroundcontrol` shows a prerelease with six packages
  and six `.sha256` files, whose notes name the commit.
  `gh api repos/mavlink/qgroundcontrol/releases/tags/daily --jq .immutable` prints `false`.
  When it prints `true`, the _GitHub daily release_ job has failed although the release is
  published, and its error says what to do ([Release immutability](#release-immutability)).
- `curl -fsS https://qgroundcontrol.s3.us-west-2.amazonaws.com/builds/master/published.json`
  names the commit and lists its `builds`.
- A red _S3 builds/_ job is AWS: [AWS setup](#aws-setup) says what each message means.
  Correct what it names and use _Re-run failed jobs_ within the three days that the
  `publish-set` artifact is kept, or `gh workflow run publish.yml -f ref=master` later.
- A green run whose _Fetch and verify_ reads "nothing to publish yet" names a package that the
  commit's failed build did not produce: re-running that build's failed jobs publishes the
  commit, and so does the next commit that builds.

**Stop it** when it misbehaves: `gh workflow disable publish.yml --repo mavlink/qgroundcontrol`.
A completed build then starts no run, and none is made up for later: after
`gh workflow enable publish.yml`, the next build that completes, or a dispatch, publishes the
newest finished commit. Disabling leaves a run that has started alone. `gh run cancel <run id>`
stops that one, and the next publication repairs what a publishing job left half done: the
record is written last, and leftover `.next` uploads are removed. To inspect what a run would
publish without writing anything, dispatch a dry run,
`gh workflow run publish.yml -f ref=master -f dry_run=true`: it decides, downloads, and
verifies as usual, and each publishing job reports what it would write. GitHub is not expected
to start a disabled workflow for a dispatch either (not tried), so dispatch the dry run before
disabling, or enable the workflow for it while no `master`, `Stable*`, or tag build is about
to complete.

#### Release immutability

GitHub's _immutable releases_ is a repository setting (Settings > General > Releases). While it
is on, GitHub locks every release when it is published. No file of the release can be added,
replaced, or deleted, and its tag can be neither moved nor deleted while the release exists.
When the release is deleted, the tag can be deleted too, but its name can never be used again
in the repository. The title, the notes, and the prerelease and latest settings stay editable.
The setting counts at publication: turning it on locks no release that was published before,
unless that release is published again, and turning it off unlocks none that was published
while it was on. Under it, GitHub's order for a release is to create a draft, attach the files,
and publish.

- **Keep it off** in a repository whose daily release Publish Builds updates, which is upstream
  and every fork that opts in. The rolling release replaces its files and moves its tag with
  each `master` build.
- **Check it** as an administrator: `gh api repos/mavlink/qgroundcontrol/immutable-releases`
  prints `{"enabled":false,"enforced_by_owner":false}` when it is off. The call takes read
  access to the repository's administration, which a workflow's `GITHUB_TOKEN` cannot be
  given, so no job can read the setting. A job sees only what a release reports of itself:
  the `immutable` field of the releases API
  (`gh api repos/mavlink/qgroundcontrol/releases/tags/<tag> --jq .immutable`).
- **Upstream** had the setting on until 2026-11-12, when it was turned off for the daily
  release. The releases that were published while it was on stay immutable: v5.1.0, v5.1.2,
  v5.1.3, v5.1.4, and v5.1.5 (read on 2026-11-12; v5.0.8 and older are not). Neither
  `force_release` nor an upload by hand (`gh release upload`, with or without `--clobber`) can
  change their files, and their tags cannot move.
- **The daily release** (`daily_release.py`) is first published as a draft that already has
  its files, so a release that GitHub locks at publication is at least complete. A release
  that is published is never sent `draft` again, since publishing a release again is what
  locks one that was published before the setting was turned on. An existing `daily` release
  that reports `immutable` is refused before any change, in a dry run too. A first
  publication that comes back immutable fails the job, although the release is published and
  complete: the job could not have known earlier. Both errors say what to do. Turn the setting
  off. Delete the release and then the `daily` tag, for which upstream's _Tag Protection_
  ruleset must be disabled or bypassed for the moment. Then give the daily release another
  tag name, because GitHub does not take `daily` again. The name is `DAILY_TAG` in
  `tools/ci/publish/daily_release.py`, and it is written out in these places besides: that
  script's docstring, which `tools/ci/README.md` repeats, the comment at the head of
  `publish.yml`, this file, `.github/ci-overview.md`, the _Daily Builds_ section of the
  Release Checklist page, the comment on version tags in `cmake/modules/Git.cmake`, the tag
  that `cmake/tests/run_git_configure_depends_test.cmake` creates, the stand-ins for GitHub in
  `tools/tests/ci/test_daily_release.py` and `tools/tests/ci/test_publish_decide.py`, and
  every download link that someone saved (`releases/download/daily/<file>`). Exercised on
  GitHub once, on a fork with the setting off and no tag ruleset (2026-11-12, under a temporary
  tag): the first publication as a draft, a re-run that finishes the draft of a failed first
  run, and an update in place that moves the tag forward. Not exercised on GitHub: both
  refusals have run against the tests' stand-in for the API only
  (`tools/tests/ci/test_daily_release.py::test_an_immutable_release_is_refused_before_any_change`,
  `tools/tests/ci/test_daily_release.py::test_a_first_publish_that_github_locks_fails_with_what_to_do`,
  `tools/tests/ci/test_daily_release.py::test_a_published_release_is_never_published_again`).
- **A tag's GitHub Release** (`release_assets.py`): an immutable release that has every file of
  the set passes, like any published release. One that lacks a file, or holds one with other
  content, fails the job with or without `force_release`. The error names the files, says
  that nothing can attach them, and points to `builds/<tag>/` on the download server, where
  the run's _S3 builds/_ job publishes the same files; a dry run reports the same. `.next`
  uploads that a failed run left on such a release stay, with a warning. A draft that is
  published as an immutable release while the job attaches is changed no further and keeps
  what it has, its `.next` uploads included. So publish a release only after its _GitHub
  Release assets_ job has passed: with the setting on, a release that was published earlier
  cannot be repaired
  (`tools/tests/ci/test_release_assets.py::test_an_immutable_release_that_has_every_file_passes`,
  `tools/tests/ci/test_release_assets.py::test_a_release_that_becomes_immutable_while_the_job_runs_keeps_what_it_has`).
- **If it is turned on again**, every release that is published from then on is locked at its
  publication. A stable release must then have its files on the draft before it is published,
  which is the order that the Release Checklist gives already. A daily release that had to be
  created again would be locked by its first publication, and every later daily build would
  fail. Whether the existing daily release goes on rolling with the setting on has not been
  tried, so keep it off.

#### AWS setup

What an AWS administrator must provide for Publish Builds. The bucket is `qgroundcontrol` in
`us-west-2` (`aws_upload.S3_BUCKET`; the region is the default of the `aws-credentials` action,
and `publish.yml` passes no other). The public reads it at
`https://qgroundcontrol.s3.us-west-2.amazonaws.com` and through a CloudFront distribution.

**Jobs and secrets.** Two jobs of `publish.yml` use AWS, _S3 builds/_ (`s3-builds`) and
_S3 latest/_ (`s3-latest`), each with `id-token: write`. No other job of it does. They read
these repository secrets:

| Secret                                       | Use                                                             |
| -------------------------------------------- | --------------------------------------------------------------- |
| `AWS_ROLE_ARN`                               | The role that both jobs assume through OIDC. Preferred          |
| `AWS_ACCESS_KEY_ID`, `AWS_SECRET_ACCESS_KEY` | An IAM user's keys, used only while `AWS_ROLE_ARN` is empty     |
| `AWS_DISTRIBUTION_ID`                        | The CloudFront distribution whose cached copies are invalidated |

Upstream has `AWS_ROLE_ARN` and `AWS_DISTRIBUTION_ID` and neither key (secret names read on
2026-11-12). With keys in place of a role, the IAM user needs the permissions below, and no
trust policy is involved. Without a role and without keys, both jobs warn, upload nothing, and
pass. Without `AWS_DISTRIBUTION_ID` the uploads go ahead and the invalidation is skipped with a
warning. A dry run asks for no credentials and starts no `aws` command. The `decide` job,
each job's `confirm` step, and by default the upload itself read `published.json` over HTTPS
as anyone can: an anonymous GET of a public object, not an AWS API call. A dry run makes the
reads of `decide` and `confirm` like any other run.

**Calls and permissions.** As `publish.yml` runs them, the jobs call AWS through two `aws`
commands (`aws_upload.py`, `common/aws.py`), and these are all the permissions that the role
needs. They are the ones that the build jobs used with it, so nothing has to be granted:

| Call                                 | IAM actions                       | Resources                                                                      |
| ------------------------------------ | --------------------------------- | ------------------------------------------------------------------------------ |
| `aws s3 cp --acl public-read`        | `s3:PutObject`, `s3:PutObjectAcl` | `arn:aws:s3:::qgroundcontrol/builds/*`, `arn:aws:s3:::qgroundcontrol/latest/*` |
| `aws cloudfront create-invalidation` | `cloudfront:CreateInvalidation`   | `arn:aws:cloudfront::<account>:distribution/<AWS_DISTRIBUTION_ID>`             |

1. `aws s3 cp <file> s3://qgroundcontrol/<dir>/<name> --acl public-read` uploads each package,
   each `.sha256`, and last the record. `<dir>` is `builds/<ref>` (`builds/master`,
   `builds/Stable_V5.2`, `builds/v5.2.0`) or `latest`. The ACL is why `s3:PutObjectAcl` is
   needed. No content type is passed: the CLI derives it from the name (`application/json` for
   the record). The CLI uploads a large file in parts, which `s3:PutObject` covers.
2. `aws cloudfront create-invalidation --distribution-id <id> --paths <path>` is sent once
   by each job, in the step after its upload (the `invalidate` command), with one wildcard
   path: `/builds/<ref>/*` by _S3 builds/_ and `/latest/*` by _S3 latest/_.

The record is read without AWS credentials: before the first upload, to see what the directory
holds, and after the record's upload, to see that anyone can read it. Both are GETs of
`https://qgroundcontrol.s3.us-west-2.amazonaws.com/<dir>/published.json`. They need the bucket
to do two things, which it does today (read on 2026-11-12): serve an object that was uploaded
`public-read` (200), and answer 403 or 404 for a key that does not exist. Nothing is listed,
copied, or deleted, so the role needs no `s3:GetObject` on `builds/` or `latest/`, no
`s3:ListBucket`, no `s3:DeleteObject`, and no action besides these. The bucket's own settings
are the ones that the build jobs relied on, which uploaded with the same `aws s3 cp` command:
it accepts ACLs, public ones included.

The role's permission policy in full for the public read, with the statement that Mirror
GStreamer needs (below):

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "UploadPublicFiles",
      "Effect": "Allow",
      "Action": ["s3:PutObject", "s3:PutObjectAcl"],
      "Resource": ["arn:aws:s3:::qgroundcontrol/builds/*", "arn:aws:s3:::qgroundcontrol/latest/*"]
    },
    {
      "Sid": "InvalidateCloudFront",
      "Effect": "Allow",
      "Action": "cloudfront:CreateInvalidation",
      "Resource": "arn:aws:cloudfront::<account>:distribution/<AWS_DISTRIBUTION_ID>"
    },
    {
      "Sid": "MirrorGStreamer",
      "Effect": "Allow",
      "Action": ["s3:GetObject", "s3:PutObject", "s3:PutObjectAcl"],
      "Resource": "arn:aws:s3:::qgroundcontrol/dependencies/gstreamer/*"
    }
  ]
}
```

**Only for the credentialed read.** With `PUBLISH_RECORD_READ: credentialed` in the `env` of
the two jobs (_S3 records_ above), `publish` and `promote` read the record before their first
upload with a third command, and the role needs two permissions more. Neither is needed, or
used, while the read is public:

| Call                                            | IAM actions     | Resources                                                                                                  |
| ----------------------------------------------- | --------------- | ---------------------------------------------------------------------------------------------------------- |
| `aws s3api get-object`                          | `s3:GetObject`  | `arn:aws:s3:::qgroundcontrol/builds/*/published.json`, `arn:aws:s3:::qgroundcontrol/latest/published.json` |
| The same call, for a record that does not exist | `s3:ListBucket` | `arn:aws:s3:::qgroundcontrol`                                                                              |

`aws s3api get-object --bucket qgroundcontrol --key <dir>/published.json <file>` is the call.
S3 answers `NoSuchKey` for a missing object only to a caller that may list the bucket, and
`AccessDenied` to every other. The job takes `NoSuchKey` alone for "nothing is published", so
without `s3:ListBucket` the first publication of every branch and tag fails. Grant it on the
bucket without a condition. The statements to add to the policy above:

```json
[
  {
    "Sid": "ReadPublishedRecords",
    "Effect": "Allow",
    "Action": "s3:GetObject",
    "Resource": [
      "arn:aws:s3:::qgroundcontrol/builds/*/published.json",
      "arn:aws:s3:::qgroundcontrol/latest/published.json"
    ]
  },
  {
    "Sid": "TellAMissingRecordFromADeniedOne",
    "Effect": "Allow",
    "Action": "s3:ListBucket",
    "Resource": "arn:aws:s3:::qgroundcontrol"
  }
]
```

Check them with the role's credentials before the line is added. This must fail with
`NoSuchKey`; `AccessDenied` means that one of the two is missing:

```sh
aws s3api get-object --bucket qgroundcontrol --key builds/none/published.json /dev/null
```

**Trust.** `aws-credentials` assumes the role with `sts:AssumeRoleWithWebIdentity`, through
GitHub's OIDC provider `token.actions.githubusercontent.com` and with the audience
`sts.amazonaws.com`. The repository uses GitHub's default subject format
(`actions/oidc/customization/sub` answers `use_default: true`, read on 2026-11-12). What the
token claims:

| Job                                                               | `sub`                                                    | `job_workflow_ref`                                                                |
| ----------------------------------------------------------------- | -------------------------------------------------------- | --------------------------------------------------------------------------------- |
| Publish Builds, whatever ref it publishes                         | `repo:mavlink/qgroundcontrol:ref:refs/heads/master`      | `mavlink/qgroundcontrol/.github/workflows/publish.yml@refs/heads/master`          |
| Mirror GStreamer, on `master`                                     | `repo:mavlink/qgroundcontrol:ref:refs/heads/master`      | `mavlink/qgroundcontrol/.github/workflows/mirror-gstreamer.yml@refs/heads/master` |
| A `Stable_V5.1` build job, for a push to the branch               | `repo:mavlink/qgroundcontrol:ref:refs/heads/Stable_V5.1` | `mavlink/qgroundcontrol/.github/workflows/linux.yml@refs/heads/Stable_V5.1`       |
| A `Stable_V5.1` build job, for the push of a tag such as `v5.1.6` | `repo:mavlink/qgroundcontrol:ref:refs/tags/v5.1.6`       | `mavlink/qgroundcontrol/.github/workflows/linux.yml@refs/tags/v5.1.6`             |

Publish Builds presents `refs/heads/master` for a `Stable*` branch and for a tag too: a
`workflow_run` run and a dispatch from the default branch execute on the default branch, and a
dispatch from any other branch is a dry run, which asks for no credentials. The build jobs that
uploaded for themselves presented the ref that they built instead: `refs/heads/master` until
this workflow replaced them, `refs/heads/Stable_V*`, and `refs/tags/v*`, each with its own
workflow file (`linux.yml`, `windows.yml`, `macos.yml`, `android.yml`) in `job_workflow_ref`.
A role that admits `master`'s build jobs today therefore admits Publish Builds. Upstream's
does: those jobs last wrote `builds/master/` on 2026-11-12. The trust policy must admit:

- `repo:mavlink/qgroundcontrol:ref:refs/heads/master`, for Publish Builds and Mirror GStreamer;
- `repo:mavlink/qgroundcontrol:ref:refs/heads/Stable_V5.1` and
  `repo:mavlink/qgroundcontrol:ref:refs/tags/v5.1.*`, for as long as releases are made from
  `Stable_V5.1`: its build jobs upload to `builds/` and, for a tag, to `latest/` with this role
  (`aws s3 cp --acl public-read` and an invalidation, and no read).

As a trust policy:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Principal": {
        "Federated": "arn:aws:iam::<account>:oidc-provider/token.actions.githubusercontent.com"
      },
      "Action": "sts:AssumeRoleWithWebIdentity",
      "Condition": {
        "StringEquals": {"token.actions.githubusercontent.com:aud": "sts.amazonaws.com"},
        "StringLike": {
          "token.actions.githubusercontent.com:sub": [
            "repo:mavlink/qgroundcontrol:ref:refs/heads/master",
            "repo:mavlink/qgroundcontrol:ref:refs/heads/Stable_V5.1",
            "repo:mavlink/qgroundcontrol:ref:refs/tags/v5.1.*"
          ]
        }
      }
    }
  ]
}
```

`Stable_V5.0` and `Stable_V4.4` present nothing. Their upload steps take the static keys only,
which upstream does not have, so they upload nothing. A branch cut from `master` after this
workflow needs no entry, and its tags need none: their packages are published from `master`.

AWS matches `aud` and `sub`. GitHub's guide to OIDC in AWS says that AWS does not support
custom claims, so `job_workflow_ref` can keep the role to `publish.yml` only as a part of the
subject (the repository's subject customization), which changes the subject of every job,
`Stable_V5.1`'s included. Nothing here needs that. The `environment: release` line of the
approval gate above would change the subject of _S3 latest/_ too, to
`repo:mavlink/qgroundcontrol:environment:release`.

**Other workflows.** `mirror-gstreamer.yml` reads the same secrets and assumes the same role,
for a `master` push that changes `.github/build-config.json` and for a dispatch, which presents
the branch it was dispatched from. It calls `aws s3api head-object` and
`aws s3 cp --acl public-read` on `dependencies/gstreamer/<platform>/<file>`, so the role also
needs `s3:GetObject`, `s3:PutObject`, and `s3:PutObjectAcl` on
`arn:aws:s3:::qgroundcontrol/dependencies/gstreamer/*`: a role narrowed to `builds/*` and
`latest/*` stops the mirror. Without `s3:GetObject` there it does not fail: it cannot see
what the bucket has, and uploads each file again. `runner-images.yml` uses another role, the
secret `RUNS_ON_AMI_ROLE_ARN`, with EC2 permissions in the RunsOn stack's account
(`.github/runner-images/README.md`), and never touches the bucket.

**Check before the merge.** Two requests that anyone can make show what the public read
depends on. The first must print `200`: an object that the role uploaded with the `public-read`
ACL is public. The second must print `403` or `404` until the first publication, and `200`
with the record after it; any other status would stop the job. Both held on 2026-11-12 (`200`
and `403`):

```sh
base=https://qgroundcontrol.s3.us-west-2.amazonaws.com
curl -sS -o /dev/null -w '%{http_code}\n' "$base/builds/master/QGroundControl-x86_64.AppImage.sha256"
curl -sS -o /dev/null -w '%{http_code}\n' "$base/builds/master/published.json"
```

The role itself is used as before: `master`'s build jobs uploaded to `builds/master/` with it
on 2026-11-12, which shows the trust for `refs/heads/master` and the two upload permissions.
The build jobs invalidated for release tags only (`/latest/<file>`), last in the `v5.1.5`
builds of 2026-10-01, whose upload steps passed (run 36878129961). The first publication is
the first to invalidate for `builds/master/`, with the same `cloudfront:CreateInvalidation`; a
role without it fails that step alone, after the upload. To see the role's own policies
without assuming it, an administrator can ask the policy simulator (it does not see the
bucket's policy or settings) and print the trust policy. Neither command was run against
upstream's account:

```sh
role=<the ARN in AWS_ROLE_ARN>
bucket=arn:aws:s3:::qgroundcontrol
aws iam simulate-principal-policy --policy-source-arn "$role" \
  --action-names s3:PutObject s3:PutObjectAcl \
  --resource-arns "$bucket/builds/master/published.json" "$bucket/latest/published.json" \
  --query 'EvaluationResults[].[EvalActionName,EvalResourceName,EvalDecision]' --output text
aws iam simulate-principal-policy --policy-source-arn "$role" \
  --action-names cloudfront:CreateInvalidation \
  --resource-arns "arn:aws:cloudfront::<account>:distribution/<AWS_DISTRIBUTION_ID>" \
  --query 'EvaluationResults[].[EvalActionName,EvalDecision]' --output text
aws iam get-role --role-name <the role's name> --query Role.AssumeRolePolicyDocument
```

Every decision must read `allowed`.

**What a failure means.** Each of these stops or warns in an S3 job alone: the daily release
and the GitHub Release assets use no AWS.

- _Configure AWS credentials_ fails with
  `Not authorized to perform sts:AssumeRoleWithWebIdentity`: the trust policy does not admit
  `repo:mavlink/qgroundcontrol:ref:refs/heads/master` with the audience `sts.amazonaws.com`,
  or the secret names another role.
- `cannot read https://qgroundcontrol.s3.us-west-2.amazonaws.com/<dir>/published.json`, with
  `HTTP 5xx`, a failed connection, or a JSON error, and
  `so nothing is uploaded: what the directory holds is unknown`: the public read of the record
  got another answer than the record, 403, or 404. Nothing was uploaded. Re-run the job when
  S3 was at fault. When the object is not a record, someone replaced it: write it again
  ([seeding](#seeding-the-latest-record) shows the form).
- `cannot read s3://qgroundcontrol/<dir>/published.json, so nothing is uploaded`, with AWS's
  `(AccessDenied) when calling the GetObject operation` and the list of permissions: only
  with the credentialed read. For a directory without a record (every new branch and tag),
  `s3:ListBucket` is missing, or `s3:GetObject` is. For a directory that has one,
  `s3:GetObject` is missing. Nothing was uploaded.
- `S3 upload failed for s3://qgroundcontrol/<key>`, with AWS's
  `(AccessDenied) when calling the PutObject operation` (`CreateMultipartUpload` for a large
  file): `s3:PutObject` or `s3:PutObjectAcl` is missing for that key, or the bucket refuses a
  public ACL. The record is written last, so the directory still records what it held before.
- `was uploaded but cannot be read back from <URL>`: the upload passed, and the public URL
  does not serve the record, so the object is not public (the bucket's public-access or ACL
  settings changed), or S3 failed at that moment, which the message then names. Do not leave
  it: the public read takes a record that it cannot read for none, so the next publish of a
  `builds/<ref>/` directory would be compared with nothing, and `latest/` would refuse every
  release. Restore the bucket's public access and re-run the job, which finds the record and
  goes on to the invalidation; a dispatch for the ref uploads everything again.
- `CloudFront builds/<ref>/ not invalidated` or `CloudFront latest/ not invalidated`, which
  says that the files and the record are in S3 and are correct and that only the invalidation
  failed, with AWS's `(AccessDenied) when calling the CreateInvalidation operation`:
  `cloudfront:CreateInvalidation` is missing, or `AWS_DISTRIBUTION_ID` names another
  distribution. _Re-run failed jobs_ sends the invalidation again, alone unless the run was
  started by hand.
- The warnings `No AWS credentials are configured` and
  `No CloudFront distribution is configured (AWS_DISTRIBUTION_ID)`, in a job that passes: the
  secret is not set.

**No longer needed.** The Linux, Windows, MacOS, and Android workflows on `master`, and on
every branch cut from it after this change, request no AWS credentials: no build job presents
a token to AWS, for a pull request or for a push
(`test_workflow_security_policy.py::test_only_publishing_and_infrastructure_workflows_hold_aws_credentials`).
The trust policy therefore needs no subject for a new `Stable*` branch or for its tags, no
`pull_request` subject (the build jobs uploaded for pushes only, so it never did), and no
wildcard such as `repo:mavlink/qgroundcontrol:*`. Once no release is made from `Stable_V5.1`
any more, `refs/heads/master` is the only subject left. The role needs no key outside
`builds/`, `latest/`, and `dependencies/gstreamer/`, and no delete permission. Upstream sets
no static keys, and `Stable_V5.0` and `Stable_V4.4`, which can use nothing else, upload
nothing, so an IAM user that was made for the keys serves no workflow.

#### Seeding the latest/ record

`latest/` holds a release that the build jobs uploaded before Publish Builds existed, and no
`published.json`. Until it has one, the `S3 latest/` job of every release tag fails after
`builds/<tag>/` is published. Do one of the following when this workflow reaches `master`.

**Which release to seed.** On 2026-11-12 `latest/` held v5.1.5, which was published on
2026-10-01, so the record to write is that of v5.1.5, unless a newer release has been published
since. Check both signs before seeding:
`curl -fsS https://qgroundcontrol.s3.us-west-2.amazonaws.com/latest/QGC.version.txt` prints the
version that the release manager wrote there as the last step of the newest release, and
`gh release view --repo mavlink/qgroundcontrol --json tagName,publishedAt` shows the newest
published GitHub Release.

**Seed again after every release from a branch that still uploads for itself**, with the same
command and that release's tag. The seed is therefore not a one-time step while such a branch
makes releases. `Stable_V5.1`
copies each tag's files to `latest/` from its own build jobs, whatever the version (the step
_Upload to S3 latest folder_ of its upload action), and writes neither a record nor checksum
files. After such a release the record names a release that `latest/` no longer holds, and
any `.sha256` files that an earlier release of Publish Builds left there belong to that
earlier release and match none of the new packages: delete them by hand. `Stable_V5.0` would do the
same (_Upload tagged stable build to S3 latest Bucket_) with the static keys, which upstream
does not have ([AWS setup](#aws-setup)). Publish Builds publishes nothing for those
branches: `Stable_V5.1`'s packages carry no marker, and `Stable_V5.0` has no `android.yml`, so
its commits stay at "waiting for Android (no run)". Their releases also get no packages on
their GitHub Release from here: attach them to the draft by hand, before it is published
([Release immutability](#release-immutability)).

- **Seed the record** of the release that `latest/` serves now, which is not the one being
  released. It takes AWS credentials that may write `latest/published.json` with a public ACL
  (`s3:PutObject` and `s3:PutObjectAcl`). `seed` prints the record and calls nothing:

  ```sh
  python3 tools/ci/publish/aws_upload.py seed --tag vX.Y.Z \
    --commit "$(git rev-list -n 1 vX.Y.Z)" > published.json
  aws s3 cp published.json s3://qgroundcontrol/latest/published.json --acl public-read
  curl -fsS https://qgroundcontrol.s3.us-west-2.amazonaws.com/latest/published.json
  ```

  The file is the record that a publication writes, without a run and without files:

  ```json
  {
    "schema_version": 1,
    "repository": "mavlink/qgroundcontrol",
    "ref": "refs/tags/vX.Y.Z",
    "commit": "<the 40-character commit of vX.Y.Z>",
    "published_at": "2026-11-12T09:00:00Z",
    "run": "",
    "files": {}
  }
  ```

  The upload is the command the workflow runs for every file: the `public-read` ACL, and the
  content type that the AWS CLI derives from the name, `application/json`. `curl` must print
  the record, because the decision and the upload read it from that URL. A publish that
  stopped for want of the record then goes on with _Re-run failed jobs_, for the three days
  its `publish-set` artifact is kept. From then on a newer release is promoted and an older
  one refused, as with any record. The seed lists no files, so the first promotion cannot
  warn of files left from the earlier uploads: compare
  `aws s3 ls s3://qgroundcontrol/latest/` with its summary once.
- **Or force the first promotion**:
  `gh workflow run publish.yml -f ref=vX.Y.Z -f force_latest=true` publishes the tag to
  `latest/` whatever that holds, and writes the record. Nothing is compared, so use it only
  for a tag that is known to be the newest release.

Never delete `latest/published.json` to get a release through: without it every later release
stops again.

#### Changing the published packages

Publish Builds runs `master`'s scripts whatever branch was built, so the names it accepts are
those in `master`'s `tools/qgc_tools/build_artifacts.py`: `PUBLISHED_BUILDS`, the packages that
`master`'s workflows mark, and `RETIRED_BUILD_NAMES`, the names they no longer mark but a
supported `Stable*` branch still does. What a branch must provide is its own copy of
`PUBLISHED_BUILDS`, which its builds record in their markers.

- **Add a package**: on `master`, in one change:
  1. Add it to `PUBLISHED_BUILDS` with the name of its workflow. Its file name must have the
     form that `build_artifacts.py` accepts (`_PUBLIC_NAME`: `QGroundControl`, any `-suffix`
     parts, and `.AppImage`, `.apk`, `.dmg`, or `.exe`), and the artifact that holds it a
     distributable name (`is_distributable_artifact`).
  2. On the `attest-and-upload` step that uploads it, set `publish` and pass
     `job-id: ${{ job.check_run_id }}`
     (`test_published_builds.py::test_published_builds_are_what_the_workflows_mark` fails while
     the list and the workflows differ, and
     `test_published_builds.py::test_every_step_that_can_mark_a_package_passes_its_job_s_id`
     without the id).
  3. When its workflow marks no package yet, add the workflow's name to `publish.yml`'s
     `workflows:` and its file to `REQUIRED_WORKFLOWS` in `publish_decide.py`
     (`test_published_builds.py::test_publish_follows_and_requires_exactly_the_workflows_that_mark_packages`).
     Every commit then waits for that workflow too.
  4. Link it in the user guide's download page, and add it to the file lists of the
     [Release Checklist](../../docs/en/qgc-dev-guide/release_branching_process.md) (what the tag
     publishes, and the loop that checks the downloads).

  A `Stable*` branch publishes it once the same change is on that branch; until then its
  releases have the packages they had. Never mark a package on a branch before `master` lists
  its name: every publish of that branch fails.
- **Remove a package**: stop marking it and move its name from `PUBLISHED_BUILDS` to
  `RETIRED_BUILD_NAMES`. Keep it there while a supported `Stable*` branch builds it; without the
  name, that branch's releases fail verification. The daily release loses the asset with the
  next publish; S3 keeps the file, as described above.
- **Rename a package**: add the new name and retire the old one, as above. The old name's files
  stay on S3 and its download links keep serving them, so change the links and delete the files.

### TestFlight

Pull-request and branch builds of `ios.yml` remain unsigned. A release tag selects the Xcode App
Store preset, imports an Apple Distribution certificate and provisioning profile, verifies the
signed bundle, and packages an IPA, which the tag's push run uploads to TestFlight. A run of the
tag that was started by hand signs and packages the same IPA, keeps it as a workflow artifact,
and says in a notice that it is not uploaded ([Release tags](#release-tags)). The job's
`APPSTORE_RELEASE` requires every variable and secret below except the bundle ID; a tag without
them builds the unsigned IPA, skips TestFlight with a notice, and does not fail
(`ios_package.py package` verifies the signature only when `APPSTORE_RELEASE` is `true`).

Repository variables: `APPSTORE_BUNDLE_ID` (defaults to `org.mavlink.qgroundcontrol`),
`APPSTORE_TEAM_ID`, `APPSTORE_ISSUER_ID`, `APPSTORE_API_KEY_ID`, and
`APPSTORE_PROVISIONING_PROFILE_NAME`.

Repository secrets: `APPSTORE_API_PRIVATE_KEY` (App Store Connect API private key in PKCS#8 `.p8`
format), `APPSTORE_CERTIFICATES_FILE_BASE64` (base64-encoded Apple Distribution `.p12`), and
`APPSTORE_CERTIFICATES_PASSWORD` (password for the distribution `.p12`).

### Secrets on pull requests

Same-repository PRs, including Renovate's, receive repository secrets even though they run
PR-controlled code. Signing and registry secrets therefore resolve empty on `pull_request`
events: Android PR builds sign with a throwaway debug keystore, macOS PR bundles are never
signed or notarized, and PR Docker builds do not log in to Docker Hub. The store uploads
(TestFlight, Play Store) run for pushes only ([Release tags](#release-tags)), and docs deploys
are gated to pushes, tags, or dispatch. The download server's credentials belong to
[Publish Builds](#publish-builds), which runs no pull request's code.
