# CI Notes: Runners

Design notes on where jobs run: RunsOn pools, managed runner images, the runner's own Python, and
runner debugging. The [CI overview](../ci-overview.md) has the contributor-facing summary.

## RunsOn Runners

- Upstream jobs (`github.repository_owner == 'mavlink'`, which includes PRs from forks) run on
  RunsOn pools defined in `.github/runs-on.yml`; workflows in independent forks fall back to
  GitHub-hosted runners and GitHub's cache. macOS, iOS, Windows arm64, and Android's macOS leg
  always use GitHub-hosted runners.
- Every RunsOn job but one requests a label of its own,
  `runs-on=<run_id>-<job id>-<run_attempt>-<matrix entry>/runner=<pool>` (Windows jobs name
  `pool=<warm pool>` instead of a runner when `RUNS_ON_WINDOWS_POOL` is set). GitHub hands a
  runner to any queued job with an identical label, so shared labels let matrix siblings steal
  each other's runners. The job id is written literally (`github.job` is empty outside steps) and
  the matrix entry is `strategy.job-index`, which matrix values cannot read, so every label is
  built in the job's `runs-on`; `linux.yml` matrix entries supply only the pool and GitHub-hosted
  fallback. The exception is the runner-image smoke test, the only RunsOn job of its run, which
  requests `runs-on=<run_id>/runner=linux-x64-builder-prebaked/ami=<candidate AMI>`.
- Pools list exact instance types in on-demand priority order, and every event, pull requests
  included, uses on-demand capacity: a reclaimed Spot instance discards a whole build (three
  Windows PR builds were lost at once on 2026-10-23). The event type never changes a job's label.
- RunsOn reads `runs-on.yml` only from the default branch, so an unknown `runner=` fails to launch
  and labels repeat settings that must apply before a `runs-on.yml` change merges (for example the
  Windows builder's 130 GB volume). The `runs-on-lint` hook runs `roc lint` on the config when it
  changes, and `test_runs_on_config.py` keeps labels and pools in sync.
- `runs-on/action` enables Magic Cache and the cost summary (plus sccache's S3 settings in
  `linux.yml` Release legs when the repository variable `QGC_COMPILER_CACHE` is `sccache`; see the
  [sccache pilot](caching.md#sccache-pilot)), and skips itself off RunsOn, so jobs call it without
  a condition; RunsOn's built-in resource charts appear in each job's "Complete runner" step, so
  paid CloudWatch metrics stay off.
- The Harden-Runner community tier audits GitHub-hosted runners only, so jobs that can run on
  RunsOn skip it there (`if: runner.environment == 'github-hosted'`).
- **apt mirror**: `apt_install_retry.py --update` points the Ubuntu archive (`archive.ubuntu.com`,
  or `ports.ubuntu.com` on arm64) at the region's EC2 mirror
  (`${RUNS_ON_AWS_REGION}.ec2.archive.ubuntu.com` or `.ec2.ports.ubuntu.com`), because the public
  archive sometimes served RunsOn jobs at about 200 kB/s or stalled `apt-get update` for minutes
  (observed for #15370 in October 2026).
  Security entries stay on their original host, as on Canonical's EC2 images, and apt still
  verifies the signed release files over plain HTTP. A mirror that doesn't resolve is not used; if
  the mirror's first update fails, the original sources are restored and the normal retries run.
  Off RunsOn it does nothing; later `apt-get` calls in the job, the Multipass installer, and the
  Vagrant step reuse it. `--update` without packages only refreshes, and the apt download cache key
  does not depend on the mirror.

## Managed Runner Images

- `runner-images.yml` (manual, plus a weekly schedule once `RUNS_ON_AMI_SUBNET_ID` is set) builds
  a QGC Ubuntu 24 x64 AMI from the RunsOn base image, smoke-tests the candidate by AMI ID, and
  promotes it only from the default branch. It uses the same dependency and Qt setup helpers as
  CI, and `qt-install` reuses the preinstalled SDK only when its Qt version, architecture, and
  module manifest satisfy the job's request. The `packer-validate` hook validates the Packer template
  on pull requests that change it, without AWS credentials.
- The managed image (`RUNS_ON_LINUX_BUILDER`) and the organization-level Windows warm pool
  (`RUNS_ON_WINDOWS_POOL`) are opt-in repository variables; unset, workflows keep the standard
  RunsOn runners. See [runner-images/README.md](../runner-images/README.md) for AWS prerequisites,
  activation, rebuild cadence, retention, and the warm-pool example.
- The Windows builder uses `windows25-full-x64`, which ships the MSVC toolchain
  (`windows22-full-x64` does not). The dependency installer skips Visual Studio Build Tools when an
  existing instance already provides every required component. The image took about 75 GB when
  measured for #15370 in October 2026 (a 100 GB volume had 24.6 GiB free and ran out), so the
  builder's volume is 130 GB, which leaves about 55 GiB for the build.

## The Runner's Python

- Steps that run before `setup-python`, or in jobs without it, use the runner image's own
  `python3`, so their scripts import only the standard library and stdlib-only `tools/` modules
  ([rule and enforcing test](../../tools/ci/README.md#shared-code-and-bootstrapping)). That
  interpreter must be at least the version in `tools/.python-version`: Ubuntu 24.04's is 3.12, and
  the Windows and macOS 15 images' are 3.12 or newer. `tools/tests/test_stdlib_only.py` fails for
  such a job on `ubuntu-22.04`, whose `python3` is 3.10.
- `setup-python` synchronizes frozen dependency groups from `tools/uv.lock` into `tools/.venv`
  (narrower calls keep already-installed groups) and adds that environment to `PATH`. uv builds it
  on the Python named in `tools/.python-version`, the only version the tools support
  ([tools/README.md](../../tools/README.md#python-environment)); the action sets no version of its
  own, and a sparse checkout that includes `tools/uv.lock` must include that file. CMake uses the
  same interpreter, and MAVLink generation uses its preinstalled dependencies without installing
  packages during a build.
- CI, Docker, VM, sysroot, and packaging glue uses Python entrypoints; shell remains only for
  installing Python itself and loading container login profiles.

## Build Timelines and Runner Debugging

- The build and test jobs of Linux (Release, coverage and sanitizers, custom plugin), Windows,
  macOS, Android, iOS, and Docker register `Kesin11/actions-timeline` before checkout, adding
  runner queueing and step timing to the job summary. Windows's `gstreamer-tests` and Linux's
  `appimage-distros` jobs register none. Each timeline is a snapshot at that job's completion;
  matrix siblings may still be running, and nested composite steps stay collapsed.
- To inspect a failed runner, manually dispatch one of those workflows on a branch with
  `debug_runner` enabled. The run has a concurrency group of its own, so it neither waits behind
  the branch's pushes nor replaces a waiting push run
  ([concurrency](../ci-overview.md#add-or-change-a-workflow)). Each failed job that has a
  timeline, and `gstreamer-tests`, opens its own actor-only `mxschmitt/action-tmate` session
  after its normal reporting steps; connect with the SSH command in the step log and a key
  registered to the dispatching account, and run `touch continue` in the checkout to finish.
  Setup and session share a 15-minute step timeout, bounded by the remaining job timeout.
- A manual run uploads to no store: the Play Store and TestFlight steps run for `push` events
  only. What it builds stays a workflow artifact, signed as a push build of its ref is.
  `android.yml` signs the APK of its primary leg with the release keystore on any ref, when
  `ANDROID_KEYSTORE_PASSWORD` is set (only pull requests are denied it), and builds no App
  Bundle, which a `master` push alone does. `ios.yml` signs for the App Store only on a release
  tag with every App Store secret and variable set, so its IPA of a branch is unsigned. A run
  dispatched on a tag that is not a `vX.Y.Z` release tag on a `Stable*` branch builds nothing
  ([release tags](platforms.md#release-tags)).
- Successful builds, PRs, tag runs, and cancelled jobs never open a session. A checkout failure can
  prevent the local action from loading; post-job cleanup failures and exhausted job timeouts
  cannot be inspected. Windows installs MSYS2 and tmate only when a session opens.
- Set `RUNNER_DEBUG=1` (re-run with debug logging) or `QGC_CI_DEBUG=1` to add the Python fault
  handler, line-buffered output, and annotations for unraisable exceptions to every CI script
  (`tools/_bootstrap.py`).

## Copilot Agent Environments

- `copilot-setup-steps.yml` prepares the Copilot cloud agent's environment: Qt, system and Python
  dependencies, restored caches, and `just configure`. Copilot uses it only from the default
  branch; run it by hand to check the setup. Sessions (event `dynamic`) restore only the default
  branch's GitHub caches and cannot save, so they skip every save; daily and input-changing master
  runs save the apt download and Qt caches they restore, which master's RunsOn builds keep in S3
  instead (sessions download CMake and Ninja). GitHub evicts both caches from the repository's
  full 10 GB pool within hours, so the master runs also upload the apt downloads as the
  `copilot-apt-debs` artifact, which sessions and manual runs restore with `apt_debs_artifact.py`
  before installing. In sessions each slow step times out within Copilot's roughly 20-minute setup
  budget, so a stall fails that step and Copilot starts without the remaining steps rather than
  cancelling the session.
- `copilot-code-review.yml` is the Copilot code review environment, used instead of
  `copilot-setup-steps.yml`: only a checkout, because the review reads the pull request without
  building it, its credential expires about 30 minutes after the run starts, and a slow Ubuntu
  mirror could fail the cloud agent's Install Dependencies step.
