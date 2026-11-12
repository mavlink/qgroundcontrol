# CI Overview

What CI runs on a pull request, the rules it relies on, and how to do the common jobs. Commands:
[AGENTS.md](../AGENTS.md), [tools/README.md](../tools/README.md). Design notes:
[caching](ci-notes/caching.md), [runners](ci-notes/runners.md),
[reporting](ci-notes/reporting.md), [platforms and workflows](ci-notes/platforms.md),
[dependencies](ci-notes/dependencies.md). Indexes: [actions](actions/README.md),
[CI scripts](../tools/ci/README.md#script-index).

## What Runs on a Pull Request

Linux, Windows, MacOS, Android, and iOS start on every pull request and merge group. Their first
job (`changes`, `_detect-changes.yml`) decides from the changed files whether the build jobs run
(`tools/ci/planning/detect_changes.py`); Docker plans its legs the same way. Pushes (unless only
`docs/` changed), release tags, merge groups, and manual runs build everything.

| A pull request that changes | Builds |
| --- | --- |
| `src/`, `test/`, `resources/`, `translations/`, `*.qrc` | The five platforms; Docker only for CMake files among them |
| `cmake/`, `libs/`, root CMake files, `.github/actions/`, `.github/build-config.json` | Everything |
| `tools/` | Everything, except `tools/tests/`, `tools/configs/` (other than `ccache.conf`), and the scripts `detect_changes.py` names as no build input (lint, analysis, reporting, release); `tools/setup/` builds the platforms each installer serves |
| Only `tools/pyproject.toml` or `tools/uv.lock` | Linux |
| `android/`, `custom-example/`, `deploy/<platform>/`, `.github/workflows/<platform>.yml` | That platform (`custom-example/`: Linux), and Docker's leg for `android/` and `deploy/linux/` |
| Markdown, `docs/`, anything else | Nothing |

A failed check blocks a merge only once a ruleset requires it, an admin setting that no file
here records; upstream required none in November 2026. What the repository provides: each
platform workflow ends in an `if: always()` gate job named after it (`Linux`, `Windows`, `MacOS`,
`Android`, `iOS`) that fails when change detection or any leg fails or is cancelled, and passes
when the builds are skipped. A ruleset should name these, not build jobs (a skipped matrix job
reports its unexpanded name), and cannot require a path-filtered workflow, which reports nothing
on a pull request that does not trigger it.

| Workflow | Runs on a pull request | Reproduce locally |
| --- | --- | --- |
| Linux, Windows, MacOS, Android, iOS | Always; builds follow change detection | `just configure`, `just build`, `just test`; more under [A check failed](#a-check-failed) |
| Docker | Unless only `docs/` changed; legs follow change detection | `just docker` |
| pre-commit | Always, and on merge groups | `uv run --frozen --project tools --group dev python tools/pre_commit.py --changed` |
| CodeQL | Always, and on merge groups | — |
| Dependency Review | Always | — |
| PR Checks | When opened, reopened, edited, or pushed to: title check (fails its `Validate PR Title` job), size and path labels (advisory) | Title: [Conventional Commits](../AGENTS.md#commit--review-conventions), subject of at most 80 characters |
| CI Scripts | `.github/`, `tools/`, `deploy/`, CMake files, tool configuration | `just test-python` |
| Code Analysis | Compiled `src/` files, `cmake/`, analysis tooling | `just analyze` |
| Check Links | `**.md`, `tools/configs/lychee.toml` | — |
| Docs | `docs/`, `.github/build-config.json` | `just docs`, `just lint-docs` |
| Doxygen API Docs | `tools/configs/Doxyfile` | `doxygen tools/configs/Doxyfile` |
| Dev Container | Image inputs, except lock-only changes | — |
| VM Builds | `deploy/multipass/`, `deploy/vagrant/`, their installers, presets | [platforms.md](ci-notes/platforms.md#vm-builds) |
| ClusterFuzzLite | `.clusterfuzzlite/`, `test/Fuzz/` | — |
| Build Results | After a `pull_request` workflow above completes, except Dependency Review, Check Links, and Docs, whose status the next one refreshes | — |
| Welcome, Cache Cleanup | A first pull request opened; a pull request closed | — |

## Workflows

PR = `pull_request`, MG = merge group, push = `master` and `Stable*` unless noted, tag = release
tag `vX.Y.Z`, manual = `workflow_dispatch`, all = those five; "(paths)" marks a path filter.
Packages = files that `publish.yml` publishes.

| Workflow | Triggers | Purpose | Publishes |
| --- | --- | --- | --- |
| `linux.yml` | all | AppImages, coverage and sanitizer test legs, custom plugin, translation extraction check | Packages; coverage to Codecov |
| `windows.yml` | all | Installers, portable and GStreamer tests | Packages |
| `macos.yml` | all | Universal DMG (signed and notarized outside PRs), Debug test leg | Packages |
| `android.yml` | all | APKs, emulator boot test, Android Lint | Packages; Play Store: beta on `master`, 20% production rollout on tags |
| `ios.yml` | all | Device IPA, simulator boot test | TestFlight on tags |
| `docker.yml` | PR, push, tag, manual | Builder images; containerized Linux and Android builds | GHCR on pushes, Docker Hub on tags |
| `devcontainer.yml` | PR and `master` push (paths), tag, manual | Build and smoke-test the Dev Container image | `ghcr.io/mavlink/qgc-dev`: `latest` on `master`, `vX.Y.Z` on tags |
| `vm-builds.yml` | PR and push (paths), manual | Builds inside the Multipass and Vagrant VMs | — |
| `flatpak.yml` | manual | Flatpak build | — |
| `_detect-changes.yml` | Called by the platform workflows and Docker | Change detection; Android and Docker matrices | — |
| `build-results.yml` | Completion of a PR workflow | Build Results comment; inline Code Analysis review | PR comment, review |
| `publish.yml` | Completion of Linux, Windows, MacOS, or Android; manual | Publish a commit's packages once all four built it | `daily` prerelease on `master`; S3 `builds/<ref>/`; `latest/`, release files on tags |
| `pre-commit.yml` | PR, MG, push | pre-commit hooks on changed files | — |
| `ci-scripts.yml` | PR and push (paths), manual | Python tests on Linux; `windows`-marked tests on Windows | — |
| `analysis.yml` | PR and `master` push (paths), manual | clang-tidy and Clazy; manual IWYU, qmllint, ThreadSanitizer | — |
| `codeql.yml` | PR, MG, push, manual | CodeQL for Actions, Java/Kotlin, Python | Code scanning |
| `clusterfuzzlite.yml` | PR (paths) | MAVLink parser fuzzing | — |
| `dependency-review.yml` | PR | Fail on new dependencies with high or critical advisories | — |
| `pr-checks.yml` | `pull_request_target` | PR title check; size and path labels | Labels |
| `check-links.yml` | PR and `master` push (paths), manual | Markdown link check (lychee) | — |
| `docs.yml` | PR and push (paths), manual | VitePress build of the guides | `mavlink/docs.qgroundcontrol.com` outside PRs |
| `doxygen.yml` | Weekly, PR (paths), manual | Doxygen API docs | `mavlink/api.qgroundcontrol.com` outside PRs |
| `pages.yml` | Completion of `master` builds, `master` push (paths), daily, manual | CI dashboard, schemas, downloads, API docs | <https://mavlink.github.io/qgroundcontrol/> |
| `release.yml` | tag, manual | Draft the GitHub Release with git-cliff notes | Draft release |
| `crowdin.yml` | `master` push (`docs/en/`), weekly, manual | Sync guide sources and translations with Crowdin | Translation PR |
| `lupdate.yml` | Weekly, manual | Extract source strings | Translation PR |
| `px4-metadata.yml` | Daily, manual | Update PX4 airframe and parameter metadata | Metadata PR |
| `mirror-gstreamer.yml` | `master` push of `build-config.json`, manual | Mirror GStreamer releases | QGC S3 bucket |
| `runner-images.yml` | Weekly, manual | Build, test, and promote the RunsOn AMI | AMI |
| `cache-cleanup.yml` | Every six hours, PR closed, manual | Delete superseded GitHub caches and compiler-cache snapshot artifacts | — |
| `build-profile.yml` | manual | CMake build profiling | — |
| `scorecard.yml` | `master` push, manual | OpenSSF Scorecard | Code scanning |
| `stale.yml` | Daily, manual | Label and close stale issues | — |
| `welcome.yml` | First issue or PR opened | Welcome comment | — |
| `copilot-setup-steps.yml` | Copilot sessions, daily, `master` push (paths), manual | Copilot cloud agent environment | — |
| `copilot-code-review.yml` | Copilot code review, manual | Checkout only | — |

## Invariants

Rules CI relies on, each with the test that fails when it breaks. Tests are in
`tools/tests/ci/contracts/` unless a path is given;
`test_scripts_docs.py::test_ci_documents_name_tests_that_exist` keeps the names valid.

| Invariant | Test |
| --- | --- |
| A platform's gate job has the workflow's name, always runs, needs every other job, and lets only conditional jobs skip | `test_required_check_gates.py::test_gate_requires_every_job_and_reports_on_every_pull_request` |
| A newer run cancels only a pull request's superseded run | `test_workflow_expressions.py::test_a_newer_run_cancels_only_a_pull_requests_superseded_run` |
| Every job has a timeout, and starts with harden-runner unless it runs only on RunsOn | `test_workflow_security_policy.py::test_executable_jobs_are_bounded_and_hardened` |
| Third-party actions are pinned to a commit SHA with a version comment; GitHub's own use a version tag | `test_workflow_security_policy.py::test_third_party_actions_are_pinned_to_commits_and_githubs_own_to_version_tags` |
| `pull_request_target` and `workflow_run` workflows check out only their own ref | `test_workflow_security_policy.py::test_privileged_workflows_check_out_only_their_own_ref` |
| A sparse-checkout job checks out every script it runs, their imports, and its local actions | `tools/tests/ci/test_bootstrap_sparse_checkout.py::test_sparse_checkout_jobs_check_out_what_they_run` |
| A job without `setup-python` runs only standard-library scripts | `tools/tests/test_stdlib_only.py::test_jobs_without_setup_python_run_only_listed_scripts` |
| Callers of a local action pass only inputs it declares, and every required one | `test_action_inputs.py::test_local_action_callers_pass_only_declared_and_all_required_inputs` |
| Push and pull-request path filters agree, except listed push-only paths | `test_workflow_triggers.py::test_push_and_pull_request_path_filters_agree` |
| `build-results.yml` triggers on every workflow that runs on pull requests, except those `REFRESHED_BY` pairs with a trigger that runs on every pull request | `test_platform_workflow_names.py::test_build_results_triggers_on_every_pull_request_workflow` |
| Every workflow a release tag starts checks that a `Stable*` branch contains the tag, in a job its other jobs need | `test_workflow_triggers.py::test_release_tag_workflows_check_the_stable_branch_before_their_other_jobs` |
| The PR title check and the `commit-msg` hook agree | `test_workflow_security_policy.py::test_pr_title_check_agrees_with_the_commit_message_hook` |
| Renovate tracks the CMake, Ninja, OpenSSL, `tools`, and AppImage pins in `build-config.json`; those with hand-updated digests wait for approval | `test_build_tools_contract.py::test_schema_requires_the_pins_and_renovate_tracks_them` |
| Renovate tracks every GitHub package CMake fetches, or the test lists it as exempt | `test_renovate_cpm_pins.py::test_every_github_cpm_package_is_tracked_by_renovate_or_exempt` |
| The locked local CMake has the major version CI pins | `test_cmake_version_alignment.py::test_the_locked_cmake_has_the_major_version_ci_pins` |
| `just test-python` and CI install the same dependency groups | `test_ci_scripts_workflow.py::test_just_test_python_installs_the_dependency_groups_ci_does` |
| Every file the CI Scripts jobs check out also triggers the workflow | `test_ci_scripts_workflow.py::test_every_file_the_test_jobs_check_out_triggers_the_workflow` |
| Every Codecov upload, in workflows and in run-unit-tests, uses OIDC, disables the default file search, passes no token, and names its files | `test_run_unit_tests_action.py::test_codecov_uploads_use_oidc_and_only_the_named_reports` |
| No build workflow, and no action a build uses, holds AWS credentials | `test_workflow_security_policy.py::test_only_publishing_and_infrastructure_workflows_hold_aws_credentials` |
| `publish.yml` follows and requires exactly the workflows that mark a package | `test_published_builds.py::test_publish_follows_and_requires_exactly_the_workflows_that_mark_packages` |

The other contract files hold one subject each: planners against their workflows
(`test_analysis_workflow.py`, `test_build_setup_action.py`, `test_cache_contracts.py`), script
steps (`test_script_steps.py`), presets (`test_cmake_presets.py`), runner pools
(`test_runs_on_config.py`), reporting (`test_build_results_workflow.py`,
`test_pages_workflow.py`), publishing (`test_publish_workflow.py`), packaging
(`test_native_packaging_contract.py`, `test_ios_packaging_contract.py`), and deploy scripts run
without containers or VMs (`test_deploy_scripts.py`, `test_vm_build_vagrant.py`).

## How-Tos

### A check failed

1. Read the PR's **Build Results** comment: a verdict, a table of every workflow, and a
   Failures section linking each failed step's log and quoting each failed test
   ([reporting.md](ci-notes/reporting.md#build-results)).
2. Open the failed job's summary. It reads top-down (Setup, Configure, Build, tests, package, boot
   test); the failing section quotes the first errors, which are also annotations.
3. `⚠️ N passed on retry` is not a failure: CI reran a failed test once and it passed. The test
   still has a bug ([test/README.md](../test/README.md#ci-retries-and-the-flaky-label)).
4. **pre-commit**: the output and the job summary end with the command to repeat the run and the
   recipe to apply fixes, `just format-fix` (clang-format) or `just format-fix-python` (Ruff).
   Add `--hook <id>` to the command in the table above to run one hook.
5. **CI Scripts**: `just test-python`. A failing run prints `--randomly-seed=N`; replay its order
   with `just test-python -n 0 --randomly-seed=N`.
6. **Unit or integration test**: `just configure && just build`, then
   `ctest --test-dir build -R <name> --output-on-failure`; `just test` runs CI's selection.
7. **Sanitizers** (needs `QT_ROOT_DIR`): `cmake --workflow --preset Linux-sanitizers`, then
   `ctest --preset Linux-sanitizers -R <name>`. **Coverage**: `just coverage`.
8. A redefinition error in an untouched file is a unity-build collision
   ([platforms.md](ci-notes/platforms.md#compiler-settings)). To inspect a failed runner, dispatch
   the workflow with `debug_runner` ([runners.md](ci-notes/runners.md#build-timelines-and-runner-debugging)).
9. **Publish Builds** fails apart from the builds. Its summary says why; re-run its failed
   jobs ([platforms.md](ci-notes/platforms.md#publish-builds)). For a failed S3 job, see
   [AWS setup](ci-notes/platforms.md#aws-setup).

### Add or change a workflow

1. **Gate job**: in a platform workflow, add a new job to the gate's `needs`, and to its
   `allowed-skips` when the job has an `if`.
2. **Concurrency**: `cancel-in-progress: ${{ github.event_name == 'pull_request' }}`, with the
   group `${{ github.workflow }}-${{ github.event_name == 'pull_request' && github.ref ||
   github.run_id }}`, so a push gets its own group and is neither cancelled nor replaced.
   Workflows whose pushes feed or overwrite per-ref publish targets (`linux`, `windows`, `macos`,
   `android`, `docker`, `devcontainer`, `docs`) group by `github.ref` instead: a ref's runs take
   turns and are never cancelled, but a waiting run gives way to a newer one. All but `docs` give
   a run started by hand its own group (`github.run_id`). `ios.yml` groups a tag's push runs by
   ref, every other run by run.
3. **Every job**: `timeout-minutes`; harden-runner first (`if: runner.environment ==
   'github-hosted'` where the job can run on RunsOn); least-privilege `permissions`;
   `persist-credentials: false` on checkouts. Pin a third-party action to its release's commit
   SHA with the version in a comment; GitHub's own take a version tag
   ([how](ci-notes/dependencies.md#github-actions)).
4. **Sparse checkouts** list directories, not scripts: `tools/_bootstrap.py`, `tools/ci`,
   `tools/common`, `tools/qgc_tools`, plus `.github/actions`, `tools/setup`, and the Python
   environment files when the job uses `setup-python`, with `sparse-checkout-cone-mode: false`
   ([details](../tools/ci/README.md#shared-code-and-bootstrapping)).
5. **Triggers**: give `pull_request` and `push` the same `paths`; add a workflow with a
   `pull_request` trigger to `build-results.yml`'s `workflow_run` list (or to `REFRESHED_BY` in
   `test_platform_workflow_names.py`, with the evidence
   [reporting.md](ci-notes/reporting.md#build-results) asks for); add a new build input to the
   patterns in `detect_changes.py`.
6. **Scripts**: put logic in `tools/ci/<package>/<script>.py` with
   `tools/tests/ci/test_<script>.py`. Regenerate the script index, which lists each script's
   callers, when a script is added, moved, or redescribed, or a workflow or action starts or
   stops calling one: `uv run --frozen --project tools python tools/ci_script_index.py`.
7. **Contract tests** (`tools/tests/ci/contracts/`) must state a cross-file invariant, sweep every
   workflow or action, or run code, and match actions by name, never by version
   ([policy](../tools/ci/README.md#tests)). Add a new invariant to the table above.
8. **Published packages**: [add, rename, or
   remove](ci-notes/platforms.md#changing-the-published-packages).
9. Validate: `uv run --frozen --project tools --group dev prek run --files <changed files>`
   (actionlint, zizmor, yamllint) and `just test-python`. Declare a new `vars.*` name in
   `.github/actionlint.yml`, or actionlint reports it.

### Add a C++ dependency

1. Declare it where it is used, with the fields in an order Renovate's CPM manager matches:
   `CPMAddPackage(NAME foo GITHUB_REPOSITORY owner/foo VERSION 1.2.3 GIT_TAG v1.2.3 ...)`.
2. If its tags are not `vX.Y.Z`, add a `packageRules` entry with `extractVersion` to
   `.github/renovate.json`. A commit pin cannot be tracked: add it to `UNTRACKED` in
   `test_renovate_cpm_pins.py` with the reason.
3. Leave the cache keys alone: `tools/ci/cache/cpm_helper.py` fingerprints every CMake file that
   declares a dependency, so the key changes by itself and builds fetch the sources until a
   Release job saves the new snapshot ([caching.md](ci-notes/caching.md#cpm-and-gstreamer-sdk-caches)).
4. Validate with `just configure && just build`, then `just test-python`. Renovate opens each
   later update as its own PR, after approval on the Dependency Dashboard.

### Add a Python dependency

1. Add it to the narrowest group under `[dependency-groups]` in `tools/pyproject.toml`.
2. Relock with a uv that meets `[tool.uv] required-version`: `uv lock --project tools`.
3. Check that CI installs the group where the import runs
   ([dependencies.md](ci-notes/dependencies.md#python-groups-in-ci)). A script that runs before
   `setup-python` cannot import third-party packages.
4. Sync and validate: `uv run --frozen --project tools python tools/setup/install_python.py dev`,
   `just test-python`, and prek on both files (Ruff, Pyright, and import-linter recheck the tree).

### Bump a pinned tool

1. Change the version in `.github/build-config.json` ([schema](build-config.schema.json)).
2. Replace its SHA-256 digests by hand, one per listed OS and architecture; nothing computes them.
3. Run `just validate-configs` and `just test-python`; the latter fails a CMake pin above the
   `lukka/get-cmake` version ([dependencies.md](ci-notes/dependencies.md#cmake-ninja-and-qt-in-ci)).
4. Of the pins Renovate tracks, those with digests wait for approval on the Dependency
   Dashboard, and their PR changes only the version: push the digests to it. Untracked pins are
   bumped by hand, and a new pin needs a schema entry
   ([dependencies.md](ci-notes/dependencies.md#renovate)).
5. An action is bumped at its `uses:` lines, the commit SHA and the version comment together;
   Renovate does it weekly ([dependencies.md](ci-notes/dependencies.md#github-actions)).

### Cut a release

Follow the [Release Checklist](../docs/en/qgc-dev-guide/release_branching_process.md). A
`vX.Y.Z` tag on a `Stable*` branch publishes to Docker Hub, the Play Store (20% production
rollout), TestFlight, and `ghcr.io/mavlink/qgc-dev:vX.Y.Z`; the store steps run for the tag's
push only and are skipped without failing when their secrets or variables are unset. Once
Linux, Windows, MacOS, and Android pass, Publish Builds uploads their packages to S3
`builds/vX.Y.Z/`, to `latest/` unless it holds a newer release and only once `latest/` has its
record (Release Checklist, pre-flight), and to the draft release. Re-pushing a tag queues a
second release run instead of cancelling the first
([platforms.md](ci-notes/platforms.md#release-tags)). Release builds compile without the compiler
and moc caches, so each takes a cold build's time ([caching.md](ci-notes/caching.md#release-tags)).
