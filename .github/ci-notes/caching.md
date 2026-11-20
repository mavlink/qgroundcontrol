# CI Notes: Caching

Design notes for the caches CI builds use: what each holds, how it is keyed, who may save it, and
why. The [CI overview](../ci-overview.md) has the contributor-facing summary; the `cache` and
`save-build-cache` actions are indexed in [`actions/README.md`](../actions/README.md) and the
scripts in [`tools/ci/README.md`](../../tools/ci/README.md#script-index).

## Backends and Planning

- Two backends: GitHub's cache, and RunsOn's
  [S3 cache backend](https://runs-on.com/docs/performance/caching/actions/) for upstream RunsOn
  jobs. They are separate; matching keys and paths cannot bridge them.
- `setup-python` runs `tools/ci/cache/cache_backend.py` from a JavaScript action, where the runner
  injects the cache URLs cache actions actually use (shell steps do not see them), and exports
  `QGC_ACTIONS_CACHE_BACKEND`. S3 requires the RunsOn proxy and its original GitHub endpoint, so a
  configured bucket alone (or a Windows stack before RunsOn 3.1) keeps GitHub limits. S3 backends
  also enable uv/Python caching with `UV_PYTHON_PREFERENCE=only-managed`, so uv caches standalone
  CPython instead of the image's Python.
- The `cache` action runs `cache_plan.py` once to resolve the save policy, cache scope, every key,
  and the compiler/moc/CPM environment. Default-branch pushes, schedules, and manual builds share
  the `shared` scope; PRs use `pr-<number>`. `pull_request_target` never saves under the default
  `auto` policy.
- Compiler, moc, CPM, and workspace Qt archives use workspace-relative paths (CPM sources live in
  `.cache/CPM`; CMake still receives an absolute `CPM_SOURCE_CACHE`), so keys match across hosted
  and RunsOn checkouts. There are no dedicated cache-warming builds, so a warm default-branch
  compiler cache is not guaranteed.
- Post-job saves (CPM, uv/Python) read environment values: nested composite step outputs are gone
  during post-job cleanup
  (`test_cache_contracts.py::test_post_job_saves_read_no_inputs_or_step_outputs_of_their_composite_action`).

## Compiler and Moc Caches

- `cache` installs compiler tools in one step: `ccache_helper.py install` installs the pinned,
  SHA-256-verified ccache for the runner's OS and architecture (Linux under `/usr/local` plus mold;
  macOS and Windows under `RUNNER_TEMP`, added to `PATH`). Docker jobs skip it
  (`install-tools: false`).
- `save-build-cache` saves once per job after a successful build or analysis autogen step
  (`cmake-build` calls it for full and `autogen` builds), before packaging, tests, or scanners can
  fail; Docker passes its build report and saves when the report confirms successful compilation.
  One `ccache_helper.py save-plan` step runs the save gate below, packs the compiler/moc snapshot,
  and names the caches to save (an empty key skips a cache), even when the gate fails; the action
  uploads the snapshot or, when the upload fails or there is none, saves those caches.
- Pull requests on GitHub storage restore the default branch's compiler, moc, CPM, and GStreamer SDK
  caches but save none of their own (`scoped_save_policy` in `cache_plan.py`): those copies cost
  upload time and, for CPM, evicted shared entries from GitHub's size-limited cache. The policy
  applies to the default `save-cache: auto`. Only Code Analysis's first shard asks to save, and
  its pull-request legs build only protocol headers, a target that saves no cache. Merge-queue
  runs save none on either backend: their temporary ref is deleted after the run, so nothing could
  restore them. On S3, PRs (forks included) save to their PR scope, but skip a compiler or moc save
  when the build missed that cache fewer than 20 times (`PR_SAVE_MIN_MISSES` in `cache_plan.py`).
  Each save uploads a multi-GB archive (measured for #15370 in October 2026: a PR build with
  99.7% hits re-uploaded 4.7 GB, 44 s on Linux and 159 s on Windows). The save gate in
  `ccache_helper.py save-plan` reads `ccache --print-stats` (Docker: the container's stats in
  `docker-build-report.json`) and the moc stats log, and saves if it can't read them or fails.
  Other events always save.
- **Snapshots (GitHub storage only)**: `build_cache_artifact.py` keeps compiler/moc snapshots as
  uncompressed workflow artifacts, outside the dependency-cache quota, so concurrent SDK and
  dependency cache writes cannot evict a just-completed build's objects. `save-build-cache` keeps
  them seven days: pull requests restore the default branch's snapshot, which a one-day retention
  expired whenever master was quiet for a day, so their builds started cold; [cleanup](#cleanup)
  deletes the snapshots that newer ones superseded. Restores accept the
  same workflow, source repository, branch, and event class; new branches can also use a trusted
  default-branch snapshot, and pull requests restore the default branch's without publishing their
  own. Names include platform, build type, variant, key suffix, and branch scope, but not the
  config hashes in the dependency-cache keys (content-addressed entries stay valid across those
  edits). Restores warn and fall back to the dependency cache on API errors, malformed responses,
  or timeouts (60 s per API attempt, 15 min for the download); after expiry a cold build is
  possible. RunsOn/S3 keeps its cache/save route.
- GitHub compiler-cache budgets are 3 GB for Windows, coverage, and sanitizer builds, 2 GB for other
  Debug and macOS builds, and 1 GB for remaining Release builds (`ci_cache_budget` in
  `cache_plan.py`); S3 backends use the `ccache.conf` ceiling, including PRs. Coverage's 3 GB
  budget is a trial: compare hits, LRU evictions, and transfer time on warm runs.
- Keys roll per run with a distinct suffix per matrix leg. moc save keys include the
  coverage/sanitizer variant while restore prefixes stay shared, so the shared moc cache holds every
  variant; the launcher's default `MOCCACHE_MAX_SIZE` is 1G (entries are mostly well-compressing
  include manifests). The job summary's Build section shows compiler and moc cache hit rates; a
  `miss-cache-evicted` row in its collapsed moc cache events means the limit is too small.
- Coverage keeps the compiler launcher (ccache stores `.gcno` files alongside objects). macOS passes
  the installed ccache binary to CMake so Homebrew discovery cannot select another version.
- Configure-check results (`check_include_file`, `check_symbol_exists`, and similar; about 650
  `try_compile` runs when counted in October 2026, mostly libarchive and SDL3) ride in the ccache
  archive under `qgc-configure-checks/`. `cmake/modules/CheckResultCache.cmake` seeds a fresh
  configure with the results saved for the same toolchain, flags, and QGC options, salted with the
  CPM dependency fingerprint and runner image (`QGC_CHECK_RESULT_CACHE_SALT`) plus, at configure
  time, the installed system packages and include paths (`system_header_digest` in
  `cmake_helper.py`: SDL3 and libarchive probe headers such as `libudev.h` unconditionally), and
  saves them after a successful configure; a local fresh configure then dropped from 29 s to 7 s
  with byte-identical generated config headers (#15370). sccache pilot legs run the checks.

## Release Tags

A release tag's packages are compiled without a compiler or moc cache. `builds_release` in
`cache_plan.py` selects a tag ref named `vX.Y.Z` (`is_release_tag` in
`qgc_tools/release_tag_check.py`), on a push or a dispatch of the tag; change detection has
already stopped the platform builds of a tag that no `Stable*` branch contains. Version markers
such as `v5.2.0-dev`, branches (`Stable*` included), and pull requests plan as before.

- **No restore or save**: the plan leaves the compiler and moc keys empty and turns the snapshot
  and `QGC_SAVE_BUILD_CACHE` off, so `cache` restores no snapshot or archive and
  `save-build-cache` skips every step
  (`test_cache_contracts.py::test_a_release_tag_plan_skips_every_compiler_and_moc_cache_step`).
  The sccache pilot is not selected, and no saved configure-check results are reused.
- **Launchers off**: `CCACHE_DISABLE=1` and `MOCCACHE_DISABLE=1` in the job environment make the
  launchers CMake configured run the real compiler and moc, without a lookup or a store. The job
  summary's Setup section then shows the compiler cache, the moc cache, and the saves as
  `off: release tag`.
- **Empty directories**: both caches live in the workspace (`.cache/ccache`, `.cache/moccache`),
  which starts empty on GitHub-hosted and RunsOn runners, each a new machine per job. The plan
  removes them anyway: a runner that keeps its workspace would have them, and a Docker build
  compiles into the mounted directory. `run_docker.py` also forwards both disable settings, so
  the launchers are off inside the container too.
- **Still restored**, because each is keyed by what it holds and a tag can restore only what its
  own ref or the default branch saved (RunsOn's S3 backend follows GitHub's scope rules): CPM
  sources (fingerprint key, one directory per package origin and version), the GStreamer SDK
  (exact key over the pinned version and checksums), Qt (version, modules, and installer digest;
  archives checked against download.qt.io's SHA-256 when installed), and apt downloads (apt
  verifies each against the signed index). The first three are trusted as restored, not hashed
  again; so are Gradle User Home on S3, whose libraries ship in the APK, the uv and CMake/Ninja
  tool caches, and Docker's layer cache.
- **Cost**: every release build is a cold build. The slowest Build steps on master in the
  [CI dashboard](https://mavlink.github.io/qgroundcontrol/) history (2026-10-18 to 2026-11-11),
  taken as cold builds, against the medians: Linux Release 17 to 18 min (3 min), Windows 21 to
  31 min (7 to 11 min), macOS universal 35 to 38 min (7 min), Android 21 to 31 min (10 to
  14 min), iOS device 23 to 24 min (8 min).

## sccache Pilot

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
  Release flags (measured for #15370 in October 2026), which slows cold builds.
- With remote storage, sccache preprocesses every source before its lookup (direct mode needs a
  local cache), about 0.4 s of CPU per hit where a ccache direct hit takes milliseconds. That offsets
  the archive transfer it removes (a warm `master` build restored 4.4 GB in 22 s and saved it in
  50 s). Both were also measured for #15370.

To evaluate, set the variable and run `linux.yml` twice on the same commit with `workflow_dispatch`
(the second run measures warm hits). Compare each leg's Build step and job durations, and the job
summary's sccache statistics (hit rate, cache writes, errors, server error log), with recent ccache
runs. sccache reaches S3 directly, so the Magic Cache traffic line in RunsOn's "Complete runner"
step leaves it out.

## CPM and GStreamer SDK Caches

- CPM uses immutable dependency-fingerprint keys (computed by `cpm_helper.py` for `cache_plan.py`):
  CMake files that declare a dependency (`CPMAddPackage`, `CPMDeclarePackage`, `CPMFindPackage`,
  `FetchContent_Declare`), option defaults, presets, `build-config.json`, the CPM modules,
  `cmake/modules/GitSparseCheckout.cmake`, and patches, so ordinary build-script edits reuse the
  default branch's snapshot. Only Release jobs save; PR and branch Release jobs first try an exact
  restore of the default-branch snapshot and skip their scoped copy when it exists. Unchanged
  snapshots are not saved again. Keys follow host and target, so Debug legs (the iOS simulator,
  Linux's custom plugin) restore their platform's Release snapshot; a `cache-key-suffix` splits them
  (Android legs), and suffixed legs fall back to the unsuffixed platform's sources. A managed runner
  image's CPM seed fills an empty cache when its fingerprint matches. Bump the keys' version
  (`CPM_KEY_PREFIX`, `cpm-sources-v3`) when a dependency's checkout shrinks or its directory
  layout changes: the restore keys carry an older archive's content into every later one.
- ArduPilot's parameter metadata lives in the CPM source cache too: `qgc_git_sparse_checkout()`
  (`cmake/modules/GitSparseCheckout.cmake`) fetches the one pinned commit and checks out only the
  parameter definitions (66 MB of a 440 MB tree at the commit pinned in November 2026) into a
  directory named for the commit, so the snapshot holds it and a complete checkout is reused
  without network access.
- GStreamer SDKs have separate immutable caches, excluded from CPM archives. Only Windows, Android,
  and iOS jobs use them (desktop Linux and macOS use the system GStreamer), keyed on the `gstreamer`
  section of `build-config.json` (fingerprinted in `cpm_helper.py`), `cmake/GStreamer/Packages.cmake`,
  `cmake/GStreamer/Acquire.cmake` and `cmake/GStreamer/acquire/`. They hold the extracted SDK trees
  without the downloaded archives; configure uses a tree stamped with the pinned checksums as is.

## Other Caches

- **Qt SDKs**, including iOS and Android target SDKs, are cached by host, target, architecture,
  version, and a digest of the modules, archive subset, and (except for aqt) installer.
  `install_qt.py plan` decides whether to save: never for a reused preinstalled SDK, a pull request
  from a fork, or a Copilot session.
- **apt**: Linux `install-dependencies` caches downloaded `.deb`s per distro, architecture, month,
  and package-list hash (`apt-debs-…`, planned by `install_dependencies_helper.py plan`); a
  `category` install uses its own `apt-debs-<category>-…` family, so it never restores into or
  replaces the full set's cache. It saves right after a successful install, except in Copilot
  sessions, which cannot write the cache; they restore the `copilot-apt-debs` artifact that
  `copilot-setup-steps.yml`'s master runs upload instead, since GitHub evicts those runs' cache
  entries within hours.
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
  (`prek cache gc`, which only warns on failure), and saves; PRs only restore. Copilot Setup Steps
  restores the same entry read-only.
- **lychee** restores and saves `.lycheecache` with a three-day entry lifetime; fork PRs only restore.
- **Docker BuildKit** layers use `type=gha,version=2`, scoped by variant and target (the Dev
  Container uses `qgc-devcontainer-<arch>`). On RunsOn, `runs-on/action` initializes
  [Magic Cache](https://runs-on.com/docs/performance/caching/docker/) before Buildx to store layers
  in S3. Only non-PR jobs export layer caches.
- **Docker application builds** use the shared `cache` action (separate from the BuildKit layer
  cache) with keys that separate image variants and compiler identities. The action skips host tool
  installation and resets/reports statistics inside the container. The workspace mount exposes
  `.cache/ccache`, `.cache/moccache`, and `.cache/CPM` at `/project/source`; only cache budgets,
  statistics settings, and a release tag's `CCACHE_DISABLE` and `MOCCACHE_DISABLE` are forwarded
  from the host environment.
- **Code Analysis results**: each `master` push runs one job over every unit and saves
  `.cache/analysis-results`; every other run restores it read-only. Keys cover the tool binary and
  version, arguments and config, compile commands without output flags, and every scanned
  dependency's contents; tool or compiler errors, timeouts, and crashes are never stored; entries
  unused for 14 days or beyond 256 MiB are pruned. To force a full recompute, bump the
  `analysis-results-v1` cache key.
- **Not cached**: Android emulator AVDs (each run creates a fresh AVD), Flatpak, and uv/Python on
  GitHub-hosted backends.

## Cleanup

`cache-cleanup.yml` (`gh_cache_cleanup.py`, built-in `gh cache` commands) runs every six hours,
when a PR closes, and on demand; the script reads each trigger's settings from the event:

- Scheduled cleanup evicts above the storage threshold, least recently used archives first, so
  newly populated larger caches survive, while protecting the latest default-branch generation of
  each build and dependency cache family, the prek hook environments, and the lychee link cache.
  Below the threshold too, it still drops all but the newest run/attempt of each pull request's
  compiler and moc cache families (`_superseded_pr_caches`), a path with nothing to act on: no
  pull-request job saves either cache on GitHub storage ([above](#compiler-and-moc-caches)).
- Closing a PR deletes every cache under that PR's merge ref (`refs/pull/<number>/merge`),
  whatever its key, without a job summary: the ones a pull request does save on GitHub storage,
  such as the link cache of each Check Links run and the apt downloads or Qt of a job that
  missed its cache.
- Manual runs default to a dry-run prune; `dry-run: false` deletes, and `mode: wipe` lists (or
  deletes) every cache of the selected branch, or of all branches.
- Deletions are scoped to the selected ref; failed deletions and an unreachable size target are
  reported. Cleanup affects GitHub storage, not the RunsOn S3 backend.

Every prune (scheduled, or a dispatch in `prune` mode, which honors `dry-run`) also deletes
superseded compiler-cache snapshot artifacts, with the `actions: write` the job already holds, so
build jobs need no permission to delete. The deletion has not run on GitHub yet: no run has
deleted an artifact with the workflow token. Only the listing and selection have run against
upstream, read-only (a dry run on 2026-11-12 found 14 snapshots, 2 of them superseded):

- **What it keeps**: the newest two (`KEEP_SNAPSHOTS`) of each artifact name and restore group,
  in a restore's own order, by artifact id. A restore group is what `build_cache_artifact.py`
  requires of a snapshot's producer (`restore_group`): the workflow, branch, repository, and
  event class, where pushes, schedules, and dispatches are one class and pull requests another
  (`tools/tests/ci/test_build_cache_artifact.py::test_a_job_restores_snapshots_of_exactly_one_restore_group`).
  A restore reads only the newest. The one before it serves a re-run of the newest's producer
  (a run skips its own snapshot), a restore that chose its snapshot before a newer one was
  saved, and the pull requests that follow a bad newest snapshot deleted by hand; a third would
  serve nothing.
- **What it can match**: only artifacts named as `artifact_name()` names them,
  `compiler-cache-v2-` and a SHA-256 (`is_snapshot_name`); expired ones are skipped.
  `test_cache_contracts.py::test_only_the_snapshot_upload_takes_a_name_the_cleanup_deletes`
  keeps every other upload off that name.
- **Listing**: it reads the repository's artifact list, newest first, back 48 hours
  (`SNAPSHOT_LOOKBACK_HOURS`: only a name saved since the last cleanup can have a superseded
  snapshot, and what a longer gap leaves expires in seven days), lists each snapshot name found
  through the API's name filter, and looks up each producing run. Upstream's unfiltered list
  held about 20,000 artifacts in November 2026 and answered some requests with an empty HTTP 500,
  so each page is tried four times. Everything is listed before anything is deleted: a page that
  keeps failing, a name's list shorter than the total the API reports, a malformed entry, or a
  failed run lookup fails the job with nothing deleted. Failed deletions are counted and warned
  about.
- **Reporting**: the log names each superseded snapshot, and the job summary's Compiler-cache
  snapshot cleanup section gives the snapshots found, those superseded, the number deleted, and
  the size freed, with the superseded ones collapsed; a dry run lists them without deleting.
- **Storage**: an upstream master push saves about 4.6 GiB under six names (2026-11-11: the
  macOS, iOS, Windows arm64, and Android-on-macOS legs). What stays is two snapshots of each
  name, about 9 GiB, plus 4.6 GiB for each push since the last cleanup: 9 to 17 GiB at 47
  master pushes a week, where seven days without cleanup hold about 216 GiB. Every other branch
  or manual scope that builds keeps at most two per name until they expire.
