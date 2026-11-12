# CI Notes: Dependencies and Pinned Tools

Design notes on where versions are pinned and how they are updated. The
[CI overview](../ci-overview.md#add-a-c-dependency) has the step-by-step how-tos for adding a C++
or Python dependency and for bumping a pinned tool.

## Where Versions Live

| What | Pinned in | Read by |
| --- | --- | --- |
| Qt, Android SDK/NDK and OpenSSL, Xcode, CMake and Ninja, GStreamer, AppImage tools, analysis toolchain, prebuilt CI tools | `.github/build-config.json` (schema: `.github/build-config.schema.json`) | `tools/setup/read_config.py`, `common.build_config`, CMake's `qgc_config_read()` |
| C++ libraries | The `CPMAddPackage` call that fetches each (`VERSION` and `GIT_TAG`) | CMake |
| ArduPilot parameter metadata | A commit of its `main` branch in `src/FirmwarePlugin/APM/CMakeLists.txt` | `qgc_git_sparse_checkout()` |
| Python packages | `tools/pyproject.toml` dependency groups, resolved in `tools/uv.lock` | uv |
| Python and uv | `tools/.python-version`; `[tool.uv] required-version` in `tools/pyproject.toml` | uv, `setup-python`, the Docker images |
| Lint tools | `.pre-commit-config.yaml` (`rev`), or the locked `lint` group for Ruff, Pyright, clang-format, and import-linter | prek |
| GitHub Actions | At each `uses:`: a commit SHA with its version in a comment, or a version tag for GitHub's own ([how](#github-actions)) | workflows and composite actions |
| Docker base images | `deploy/docker/variants.json` (tag and digest) | the Dockerfile, `docker-compose.yml` |

`build-config.json`, how to read it, and `just validate-configs` are described in
[tools/README.md](../../tools/README.md#centralized-configuration); what each CMake-side pin feeds
in [cmake/README.md](../../cmake/README.md#conventions).

## CMake, Ninja, and Qt in CI

- **CMake and Ninja**: every CI build job runs the `build.cmake_version` and `build.ninja_version`
  that `build-setup` installs with `lukka/get-cmake`, after `setup-python` so they precede any CMake
  in `tools/.venv`; `install-dependencies` installs neither, nor the distro ccache and mold
  (`CI_PROVIDED_TOOLS` in `install_dependencies_helper.py`; `cache` installs pinned ccache and
  mold). get-cmake resolves versions from the catalog bundled in its release, so the CMake pin
  cannot exceed the action's version, the comment after its commit SHA;
  `test_build_tools_contract.py::test_pinned_cmake_is_in_the_get_cmake_catalog_and_meets_the_minimum`
  fails when it does.
- **Local CMake**: local development uses the `build` group's CMake, Ninja and just
  (`install_dependencies` syncs it on Linux), and the `just` recipes run that CMake through
  `uv run --group build`. `tools/pyproject.toml` allows `cmake>=3.28,<5` and `tools/uv.lock` locks
  the version CI pins; Renovate updates the two from different sources, so
  `test_cmake_version_alignment.py` ties only their major version, where CMake removes behaviour
  and changes policy defaults.
- `build-setup`'s planner outputs the versions its steps need, and `qt-install` reads the Qt
  version itself, so its callers pass none.
- **Qt installer**: `qt-install` runs `install_qt.py`, which installs Qt with QGC's installer
  (`tools/setup/qt_installer`, standard library plus `cmake -E tar`); aqtinstall stays as the
  fallback through `QGC_QT_INSTALLER`. Every job reaching `qt-install` has CMake: `build-setup`
  installs it first, `pre-commit.yml` syncs the `dev` group, and `lupdate.yml` uses the
  GitHub-hosted image's ([tools/README.md](../../tools/README.md#qt-installer)).

## Python Groups in CI

`setup-python` installs the groups a job asks for (default `scripts`) from `tools/uv.lock`
([runners.md](runners.md#the-runners-python)). The Python test job installs `test`, `terrain`,
`simulation`, and `release`, so the terrain pipeline, mock vehicle, and release-notes tests run
instead of skipping; `just test-python` installs the same four
(`test_ci_scripts_workflow.py::test_just_test_python_installs_the_dependency_groups_ci_does`).
`test` names the `cmake` package too, leaving its version range to `build`:
`tools/tests/test_validate_cmake_presets.py` checks the presets against the schema that package
ships, and fails without it (when only `build` provided the package, those tests skipped in CI).
The Windows test job and Check Links also install `test`; the locked CMake has Windows x64 and
ARM64 wheels. The `dev` group includes all but `terrain`, whose rasterio needs a wheel for the
platform (macOS 15 or newer, glibc 2.28 or newer, Windows) or a source build against GDAL, so the
first setup step needs neither; the Pyright hook and `just test-python` name `terrain` themselves.
`pre-commit.yml` installs `dev`; build jobs install `scripts`, plus `coverage` on the coverage
leg. Group contents are listed in [tools/README.md](../../tools/README.md#python-environment).

## GitHub Actions

Every action and reusable workflow outside `actions/*` and `github/*` is referenced by a full
commit SHA, followed by a comment with the version that commit is tagged. GitHub's own stay on
version tags:

```yaml
uses: step-security/harden-runner@1d88384fd173af6ecb012dfd0264a306b711241a # v2.22.4
uses: gradle/actions/setup-gradle@8da46b539cd8fe6ab27ca75128919a18d757604c # v6.4.1
uses: actions/checkout@v7
```

Whoever controls an action's repository can move a tag, and a job that names the tag then runs the
new commit at once, without a pull request; a commit SHA always names the same code. The
[policy](../actions/README.md#external-action-reference-policy) lists the checks that enforce the
split.

- **Updates**: Renovate reads the version from the comment and replaces the SHA and the comment
  together, in the weekly grouped PR, seven days after the release is published. A PR that changes
  a SHA and leaves its version alone means the tag was moved to another commit. The cooldown
  cannot be relied on to hold that back, because Renovate dates a tag by its commit's own date, so
  read the action's diff before merging it.
- **Security fixes**: GitHub raises Dependabot alerts only for actions referenced by version,
  not by SHA, so a pinned action gets no alert and no part in Renovate's security PR. Its fixed
  release arrives with the weekly update, after the cooldown; bump it by hand to take one sooner.
- **Version comment**: the most specific version tag on the commit (`# v2.22.4`, not `# v2`), so
  that a release, not a moved major tag, is what updates the pin. `google/clusterfuzzlite`
  publishes only `v1`, so its comment is `# v1` and each move of that tag arrives as a SHA-only
  change.
- **Adding an action, or bumping one by hand**: pick a release tag and resolve it to its commit
  with `gh api repos/<owner>/<repo>/commits/<tag> --jq .sha`, then write
  `uses: <owner>/<repo>[/<path>]@<sha> # <tag>`. That call follows an annotated tag to its commit;
  `git ls-remote <url> 'refs/tags/<tag>^{}'` prints the same SHA, and without `^{}` prints the tag
  object, which is not the commit. Some repositories publish their major version as a branch, not
  a tag (`runs-on/action`'s `v2`): use the release tag at its head. A third-party action added by
  tag fails zizmor and the contract test.

## Renovate

Renovate (`.github/renovate.json`) is the only update bot. Dependabot only raises the security
alerts behind Renovate's security fixes; keep Dependabot security updates turned off in the
repository settings, or both bots open security PRs. `.github/dependabot.disabled.yml` is a standby
configuration that GitHub ignores until it is renamed to `dependabot.yml`. It would cover the
actions, npm, Python (uv), pre-commit, the Dev Container features, the fuzzing Dockerfile's base
image, and the Android Gradle libraries. It would not cover anything Renovate tracks with a
regex manager (the Docker variant base images, the uv floor, the `build-config.json` pins, the
CPM libraries, and ArduPilot's parameter repository), the builder Dockerfile, `docs/.nvmrc`,
lock file maintenance, or holding an update for approval. Its header gives the steps to switch
and the full list. Each rule's reason is its `description` in `renovate.json`.

- Renovate covers `npm`, Python (pep621/uv), pre-commit, devcontainer, Dockerfile, Node
  (`docs/.nvmrc`), Android Gradle, and the actions in workflows and composite actions. Regex
  managers add the Docker variant base images, the uv `required-version` floor (from uv's PyPI
  releases), the pins in `build-config.json` (the CI CMake, following get-cmake's tags, Ninja, the
  `tools` releases, the Android OpenSSL release, and the AppImage tools), the C++ libraries that
  `CPMAddPackage` fetches from GitHub at a version tag, and ArduPilot's parameter repository.
- Updates arrive as one weekly grouped PR (before 06:00 UTC on Monday), with Android libraries in
  a second and lock file maintenance (`tools/uv.lock`, `docs/package-lock.json`) in a third on
  the same schedule. Renovate merges nothing itself, and major updates wait for approval on the
  Dependency Dashboard issue. An action release is adopted only 7 days after it is published, and
  a Ninja release after 30. Third-party actions are pinned to commits (`pinDigests`,
  [above](#github-actions)). Runner images and job containers in workflows are not updated.
- An open Renovate PR changes only inside its schedule (`updateNotScheduled: false`) and is
  rebased only when it conflicts (`rebaseWhen: conflicted`): by default Renovate pushed the grouped
  PR again for every release of a grouped dependency and after master moved, and each push runs
  every platform pipeline. To update a PR sooner, tick the rebase checkbox in its description.
- Security fixes arrive at any time, all in one PR, at the newest fixed version; that PR is always
  in schedule. It cannot include an action pinned to a commit ([above](#github-actions)).
- Ubuntu base images take digest updates only, because each Ubuntu variant is named for its
  release, and a base-image bump also updates the build args pinned in
  `tools/tests/ci/test_plan_docker_builds.py`. The aqtinstall git pin in `tools/pyproject.toml` is
  not updated.
- The CPM libraries ship in the application, so each waits for approval on the Dependency Dashboard
  and gets its own PR. Pins Renovate cannot follow as versions (commits, a tag without a version)
  and one held back on purpose are listed in `test_renovate_cpm_pins.py`, which fails when a
  GitHub-hosted package is neither tracked nor listed. No pin tracks a branch head any more:
  ArduPilot's parameter repository is pinned to a commit of `main`, and each new commit arrives as
  its own weekly PR without approval.
- The Qt-coupled Android toolchain (the Android Gradle Plugin, the Gradle wrapper, the Kotlin
  Gradle plugin, and `androidx` libraries) also waits for approval there, as its own
  `android toolchain` group: Qt's Android template
  (`<Qt>/<android kit>/src/android/templates/build.gradle` and `src/3rdparty/gradle`) pins these per
  Qt release, and a newer release can need a compileSdk or Android Gradle Plugin newer than Qt
  supports, breaking `androidx` and other libraries. Update them with Qt, following its template.
- Pins that carry SHA-256 digests wait for approval there and then open their own PR, which must
  add the new release's digests by hand before its checksum checks pass: the `build-config.json`
  `tools` pins, the Android OpenSSL release (held to the 3.5 long-term support series), linuxdeploy,
  and appimagetool. appimagelint is not updated: it publishes only a rolling `continuous` release,
  so `build-config.json` pins one reviewed build by digest.
  `test_build_tools_contract.py::test_schema_requires_the_pins_and_renovate_tracks_them` keeps the
  tracked set equal to these pins plus CMake and Ninja. Renovate does not track the other
  `build-config.json` values (Qt, the Android SDK and NDK, Xcode, GStreamer, the analysis
  toolchain); bump them by hand (`just check-deps` and `just check-gstreamer` report newer Qt and
  GStreamer releases).
