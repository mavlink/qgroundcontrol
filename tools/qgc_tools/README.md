# QGC tooling domains

Reusable repository policy lives here; CLI and platform entrypoints call it.

- `python_env.py`: locked dependency profiles, environment and executable resolution.
- `workflow_runs.py`: validated workflow fields, run selection, grouping, and cached inputs.
- `docker_variants.py`: Docker variant manifest and validation shared by CI and deployment.

This package may import `common`, but not `setup`, `analyzers`, or `generators`.
`common` must not import this package or any command entrypoint. Run
`cd tools && lint-imports --config pyproject.toml` from the developer environment
when changing these boundaries.
