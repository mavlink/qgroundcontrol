# QGC tooling domains

Reusable repository policy lives here; CLI and platform entrypoints call it.

- `build_profile.py`: Ninja log and Clang time-trace hotspot summaries and CI build-timing snapshots.
- `cmake_configure.py`: configure-preset selection and `qt-cmake` invocation for `configure.py` and CI.
- `python_env.py`: locked dependency profiles, environment and executable resolution.
- `docker_variants.py`: Docker variant manifest and validation shared by CI and deployment.
- `release_tag_check.py`: whether a tag is a `vX.Y.Z` release tag reachable from a `Stable*` branch,
  shared by change detection, build planning, and publishing.

This package may import `common`, but not `setup`, `analyzers`, `generators`, `terrain`, or
`translations`.
`common` must not import this package or any command entrypoint. Run
`cd tools && lint-imports` from the developer environment
when changing these boundaries.
