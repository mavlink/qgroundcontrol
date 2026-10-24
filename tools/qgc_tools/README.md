# QGC tooling domains

Reusable repository policy lives here; CLI and platform entrypoints call it.

- `build_artifacts.py`: which GitHub artifacts are distributable packages (the build-results
  comment's and the Pages dashboard's sizes), and the packages S3 publishes to `builds/<ref>/`
  (the Pages downloads page).
- `build_profile.py`: Ninja log and Clang time-trace hotspot summaries, CI build-timing snapshots,
  and their job-summary tables.
- `build_errors.py`: the errors that explain a failed configure or build in its output (CMake
  errors, Ninja and Xcode failed targets, GCC/Clang/javac/MSVC compiler and linker errors, Gradle
  failures) and their checkout-relative paths, shared by `cmake_helper.py` and `android_lint.py`.
- `compiler_cache.py`: ccache and sccache statistics for build summaries and cache-save gates.
- `cmake_configure.py`: configure-preset selection and `qt-cmake` invocation for `configure.py` and CI.
- `python_env.py`: locked dependency profiles, environment and executable resolution.
- `docker_variants.py`: Docker variant manifest and validation shared by CI and deployment, and
  the builder image's registry push target for an event.
- `release_tag_check.py`: whether a tag is a `vX.Y.Z` release tag reachable from a `Stable*` branch,
  shared by change detection, build planning, and publishing.

This package may import `common`, but not `setup`, `analyzers`, `generators`, `terrain`, or
`translations`.
`common` must not import this package or any command entrypoint. Run
`cd tools && lint-imports` from the developer environment
when changing these boundaries.
