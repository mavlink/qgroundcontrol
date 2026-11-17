# CI Notes: Reporting and Publishing

Design notes on what a build reports and publishes: job summaries, annotations, the Build Results
comment, artifact attestation and upload, the artifact report, and the GitHub Pages site. The
[CI overview](../ci-overview.md#a-check-failed) explains how to read them when a check fails.

## Job Summaries

Scripts build each job-summary section with `common.markdown.summary_section`: an H3 title, an
`Item | Value` facts table, an optional body, and collapsed `<details>` for long listings, with
numbers, durations, and sizes from `common.format`. A build job reads top-down:

| Section | Written by |
| --- | --- |
| Setup | `build-setup` (`setup_summary.py`, also after a failed setup step, which it names): runner, image (RunsOn image and AMI, or the hosted `ImageOS` and `ImageVersion`), RunsOn instance type, lifecycle, zone and ID, CPUs and RAM, Qt version, installer and source, CMake and Ninja, compiler, Xcode, JDK, Android NDK, Python groups, cache backend, and each build cache's restore result with ccache's primary key, and the key a fallback (restore-keys) hit came from |
| Configure | `cmake-configure` (`cmake_helper.py configure`, also after failures): result, duration, preset, generator, configure-check reuse; after a failure the first CMake error and the end of the output collapsed, and a link to the configure logs |
| Build | `cmake-build` (`cmake_helper.py build`, also after failures): result, duration, toolchain, this build's compiler (ccache or sccache) and moc cache hit rates; failed targets and first errors, slowest steps, time by kind, and moc cache events collapsed; a link to the Ninja reports |
| Android Lint | `android_lint.py` on Android's Linux leg (also after failures): result, duration, error and warning counts; the first errors collapsed; after a failure a link to the lint reports |
| APK check | `android_apk_check.py` on every Android leg: result, ABIs, native library count, problem count, zipalign result; the problems collapsed |
| Unit Tests / Integration Tests (*suite*) | `run-unit-tests` (`junit_report.py`): counts, failures with their first lines, tests that passed on retry, slowest tests and failure output collapsed; a link to the results artifact |
| Coverage, Binary size | `coverage_report.py --step-summary`, `size_analysis.py` |
| Artifact report | `artifact-report` (`artifact_report.py`, after packaging): result, package, master baseline run, files added, removed, and changed; package and contents sizes against master; GPL findings (a known issue with `gpl-findings: warn`); added and removed files and the largest size changes, with the full lists collapsed |
| Boot test, Artifact | `verify_executable.py` (result with the crash's signal or NTSTATUS name, package, size, duration; after a failure the last log line, the last 40 lines collapsed, and a link to the diagnostics artifact) or, for the iOS simulator, `ios_boot_test.py` (result with the failing phase and attempt, simulator, attempts, duration, and the end of the boot log); and `attest_helper.py prepare` (file, size, SHA-256) |
| Disk | `disk-watch`'s post step, at the end of every `build-setup` and Docker job: the workspace volume's size and its start, peak (with free space and time), and end use, sampled every 10 s; a peak whose free space is below 10% of the volume, kept between 5 and 10 GiB, gets ⚠️ and a "Disk nearly full" warning annotation |

Docker builds write one Docker build section (`docker_helper.py summary`); pre-commit,
Code Analysis (`analyze.py --step-summary`), cache cleanup, GStreamer, release-notes, and Build
Profile jobs use the same layout. RunsOn's cost table and the build timeline come from their own
actions after these sections. What each action uploads after a failure is listed in
[`actions/README.md`](../actions/README.md#action-notes).

## Annotations and Logs

- In GitHub Actions, an uncaught exception in a CI Python entrypoint becomes an error annotation
  (`tools/_bootstrap.py`); for a failed command it names the command and quotes the last ten lines
  of its captured stderr (else stdout), with URL credentials and `Authorization` values hidden,
  and stdout escapes characters that the Windows code page cannot encode.
  `common.proc.run_with_retry` logs the same before raising its final failure.
- A failed configure annotates its first `CMake Error`; a failed build annotates its first
  compiler, linker, or Gradle errors; failed tests become error annotations, and a test that
  passed on retry a "Flaky test" warning.
- Routine timings ("Configure completed in 12s", "Tests completed in 71s") are plain log lines,
  not notices: the job summary already reports them, and annotations are kept for problems.
- Scripts reach the GitHub API through the `gh` CLI via `common.gh_actions` (`gh()` calls without
  an explicit `timeout` stop after 120 s; artifact downloads set their own) and write step outputs
  with `common.gh_actions.write_github_output()`.

## Test Results and Coverage

- `test-phase` runs the Unit suite, then the Integration suite even if Unit failed; each keeps
  its JUnit file, CTest output, and job-summary section. `cmake_helper.py ctest` reruns failed
  tests once and reports a test that then passes as "passed on retry" instead of failing the job;
  when it retries, and what the reports show, is described in
  [test/README.md](../../test/README.md#ci-retries-and-the-flaky-label). Empty test selections
  fail (`--no-tests=error`).
- Set the repository variable `CODECOV_TEST_ANALYTICS=true` to upload JUnit results from the Linux
  test jobs (including the custom plugin's) to Codecov test analytics (OIDC, informational). JUnit
  artifacts and job summary reports are produced either way.
- **Coverage**: GCC uses atomic counter updates for worker threads; gcovr tolerates the known GCC
  negative-counter bug and reports discarded counters. Codecov receives only the Cobertura
  `coverage.xml`, over OIDC (fork PRs upload tokenless), and posts its pull request comment but
  no `codecov/project` or `codecov/patch` check (`.github/codecov.yml`): both were informational,
  and Codecov sometimes left one "in progress" for good. The step summary, threshold check, and
  build-results comment read gcovr's `coverage-summary.json`. `coverage_report.py` fails before
  generating them when the tests wrote no `.gcda` counters. `verify_coverage_thresholds.py
  --build-dir` reads thresholds from `CMakeCache.txt` and compares exact covered/total counts, not
  gcovr's rounded percentages.

## Build Results

- `build-results.yml` runs on `workflow_run` completion of the workflows with a `pull_request`
  trigger: the platform workflows, pre-commit, and Code Analysis, whose artifacts the comment
  renders, and the other checks, which each add a status (GitHub runs only the default-branch
  copy). Only pull request builds start a job: the first job's `if` reads the triggering event
  from the payload and the other jobs need it, so a push, schedule, dispatch, or merge-queue
  build's run ends with every job skipped, on no runner
  (`test_build_results_workflow.py::test_only_a_pull_request_build_starts_a_job`). One
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
  also triggers the comment, so a late failure still reaches the verdict, except the three below.
- **Triggers**: Dependency Review, Check Links, and Docs do not trigger the comment. Each adds
  only a status, and in upstream's pull request runs of 2026-10-29 to 2026-11-11 none was the
  last workflow of its commit to complete (204, 72, and 119 runs; MacOS was last on 36% of the
  204 commits, iOS on 30%, Android and Windows on about 10% each, VM Builds on 12% of those it
  ran on, and CodeQL, Linux, Docker, Code Analysis, and CI Scripts on 3% or less). They ended a
  median 22, 27, and 50 s after the push (at most 324, 45, and 70 s), and a remaining trigger
  completed a median 28 to 47 s later (at most 108 s): CodeQL runs on every pull request, with
  no path filter, and never took under 109 s. Leaving them out removes 413 of that fortnight's
  2,654 Build Results runs (16%; they were 43% of all workflow runs) and as large a share of
  their runner time (39 s a run on average: a 14 s context job, then a 33 s comment job). The
  first comment now follows a push by a median 70 s instead of 22 s. A status stays stale only
  if one of the three ends after every trigger, as when it is re-run once the others have
  finished (no such re-run in the fortnight); re-running any trigger refreshes it.
  `REFRESHED_BY` in `test_platform_workflow_names.py` names the three with CodeQL, and
  `test_platform_workflow_names.py::test_build_results_triggers_on_every_pull_request_workflow`
  fails for a pull-request workflow that is neither a trigger nor listed there, for an
  artifact-rendering workflow that is not a trigger, and for a refresher with a filtered
  `pull_request` trigger. Every other trigger stays, including those rarely last: pre-commit's
  duration overlaps CodeQL's, and ClusterFuzzLite, Dev Container, and Doxygen run for minutes on
  pull requests that skip the platform builds.
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
  hook counts (with `⚠️ N passed on retry` when CI's retry rescued a test), size change from the
  base, and run link; Code Analysis shows its warnings on lines the pull request changed. Lines
  follow for the other checks (problems linked first) and for the base commit with each platform's
  base run. A Failures section lists each failed job with links to its failing steps' logs (or the
  step its runner was lost during), failed tests with their first report lines (QtTest `FAIL!`
  blocks, sanitizer reports, else the output tail) from the JUnit XML, failed pre-commit hooks,
  and failed other checks. Collapsed sections cover per-suite test results with the tests each
  suite added or removed versus the base build and the tests that passed on retry; `src/` line,
  function, and branch coverage with changes in percentage points (test code is left out, as on
  Codecov, which the section links for the changed lines' coverage); artifact downloads with their
  size change (B, KB, or MB; ⚠️ above 1% and 1 MiB growth) and expiry; pre-commit counts when a
  hook failed; and Code Analysis's warnings on changed lines (`changed` in each
  `*-diagnostics.json`), bug-class ones first and all counted by check, linked at the commit the
  analysis checked out (`commit`, GitHub's merge
  commit on a pull request, whose line numbers can differ from the head's; the head commit for
  reports without one), with every unique warning counted only in a per-tool table. Lists are
  truncated with "…and N more", and longer reports shrink to stay under GitHub's comment limit.
  Names from pull requests (tests, jobs, hooks, artifacts) are escaped or shown as code. Android
  artifact names include their ABIs, so single-ABI PR APKs are not compared with multi-ABI
  release APKs; unmatched variants show `—` for their change.
- `report_context.py --comment comment.md` rechecks that the PR head is still the build's commit,
  then creates or updates the PR's comment marked `<!-- build-results -->` (or the marker of the
  comments `thollander/actions-comment-pull-request` posted before) through `gh api`.
- When the triggering run is Code Analysis, a separate `analysis-review` job (in its own
  `analysis-review-pr-<N>` concurrency group, so a trigger queueing behind the comment job cannot
  cancel it) downloads the triggering run's `clang-tidy-clazy-output` artifacts, and
  `analysis_review.py` posts their bug-class findings (`is_bug_class`: errors, and clang-tidy's
  `bugprone-*` and `clang-analyzer-*` warnings) on lines the PR's diff adds as one inline
  `COMMENT` review on the PR head (each finding once, with its escaped message, bare URLs and
  mentions broken, and fenced source excerpt, and marked so a later run never repeats it, even
  after its line moves). A pull request gets at most 10 such comments over all its reviews,
  counting those it has, outdated ones too
  (`tools/tests/ci/test_analysis_review.py::test_a_pull_request_gets_ten_finding_comments_over_all_its_reviews`);
  errors are posted past that, at most 25 in one review, and warnings take turns by check. Other
  warnings (`performance-*`, Clazy) are only in the Build Results comment, which the review
  points to, and it says its results are partial when the analysis run did not succeed. Those
  rows carry each
  diagnostic's message and excerpt (`changed` in `*-diagnostics.json`); files that one
  `compare/<head>...<merge commit>` call shows the merge changed are skipped, since their line
  numbers can differ (past the 300 files that call lists, each file's blobs are compared instead).
  The job needs only the standard library and only warns on failure.

## Attestation and Upload

- `attest-and-upload` runs `attest_helper.py prepare` (verifies or writes the artifact's `.sha256`
  sidecar and writes the CPM dependency SBOM and a `.build.json` manifest with producer identity,
  checksum, and selected CMake configuration), scans an SPDX SBOM, attests provenance (of the
  artifact, the manifest, and an AppImage's `.zsync` when `companion-path` names one) and SBOM
  with `actions/attest` (separate invocations), and uploads the GitHub artifact. Pull requests skip the SBOM scan and attestations but still upload the checksum,
  dependency SBOM, and build identity.
- The action publishes nothing and takes no credentials. With its `publish` input, `prepare`
  marks the package in the `.build.json` manifest, and the Publish Builds workflow publishes the
  marked packages of a commit once every platform has built it: a branch's also when a job that
  builds no package failed, a release tag's only when every workflow passed
  ([platforms.md](platforms.md#publish-builds)).

## Artifact Report

- Every platform build job runs `artifact-report` (`tools/ci/packaging/artifact_report.py`) right
  after creating the package that `attest-and-upload` uploads, under the same condition: the Windows
  installers and the macOS DMG of Release legs, the Linux AppImages, the Android APKs (not the
  emulator leg), and the iOS IPA. It lists the package with tools the runners have: 7-Zip
  (`7z l -slt`) for the NSIS installer, `hdiutil` for the DMG, the AppImage's own
  `--appimage-extract`, and Python's `zipfile` for APK, App Bundle, and IPA.
- The baseline is the package in the artifact of the same name from the newest successful
  non-PR master run of the same workflow (`GITHUB_WORKFLOW_REF`), found and downloaded through the
  REST API with the job's token, which needs `actions: read`. The job summary's Artifact report
  section shows the package and contents sizes against master with their change, the files added
  and removed (the largest 10; the full lists collapsed), and the 10 largest file size changes. A
  package or contents size change beyond the `size-threshold` input (10% either way by default) is
  a warning, never a failure. Without a baseline (no successful master run with a live artifact of
  that name, as on a first run, after the 30-day retention, or for Android PR legs whose ABI suffix
  master does not build, or an API or download failure) the section says why and shows this
  build's sizes only.
- The step also checks licensing and fails with error annotations: any file in the package whose
  name is a GPL-denylisted library or plugin (`gstreamer_plugin_policy.gpl_denylist` in
  `.github/build-config.json`, matched like `PluginPolicy.cmake`'s `qgc_gst_gpl_files()`, which the
  CMake install check uses only for the GStreamer install directories), and, in APK, App Bundle,
  and IPA, whose static GStreamer leaves no plugin files, any ELF or Mach-O member containing
  x264's or x265's entry point (`x264_encoder_open`, `x265_encoder_open`) or the banner it writes
  into every stream, which stripping keeps. A package the step cannot list (for example without
  7-Zip) fails it too, since its GPL check did not run. It uploads nothing.
- **Known issue**: the Linux AppImages ship GPL `libx264` and `libx265`. Ubuntu's `libavcodec`,
  which the bundled gst-libav plugin (`libgstlibav.so`) loads, links them, and the AppImage's
  dependency deployment bundles them outside the GStreamer directories that the install check
  covers. Until that is fixed, `linux.yml` sets the action's `gpl-findings` input to `warn`: the
  findings become warning annotations and the section lists them first as a known issue with this
  explanation (`KNOWN_GPL_ISSUES` in `artifact_report.py`). Every other package keeps the default
  `fail`.

## GitHub Pages Site

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
  URL there (the build and `tools/tests/ci/test_pages_site.py` check it), with an index page.
- **Downloads** (`downloads/`): the packages in `qgc_tools/build_artifacts.py`'s
  `PUBLISHED_BUILDS` under S3 `builds/master/` (`test_published_builds.py` keeps the list in
  sync with the workflows' `attest-and-upload` steps and the user guide), with the SHA-256 from
  each `.sha256` sidecar ("—" when missing) and the commit and run named by the package's
  provenance attestation, looked up by digest, plus `gh attestation verify` instructions.
- **API docs** (`api/`): the `doxygen_build` artifact of the latest successful non-PR Doxygen run
  on master, else (or when its download stalls past two 150-second attempts) a page linking
  <https://api.qgroundcontrol.com>. The Doxygen workflow still deploys that domain; moving it to
  the Pages site is a later admin step. Master Doxygen runs keep their HTML artifact eight days.

It runs on `workflow_run` completion of the platform workflows, Code Analysis, and Doxygen on
`master`, on master pushes that change the schema or the site tooling, daily, and on dispatch;
never on pull requests, whose artifacts must not reach the site (CI Scripts tests the tooling).
The build job runs only in `mavlink/qgroundcontrol` and, for `workflow_run`, only for a master
push run of this repository (Doxygen's scheduled or manual runs for Doxygen); it checks out the
default branch's tooling with `contents: read` and `actions: read`. The deploy job alone has
`pages: write` and `id-token: write`, in the `github-pages` environment. One `pages` concurrency
group serializes deploys without cancelling them. Pages must be enabled with source "GitHub
Actions" (Settings → Pages) before the first deploy.
