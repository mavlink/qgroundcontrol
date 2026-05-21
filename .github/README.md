# `.github/` — CI, Workflows, and Repo Metadata

Platform workflows (`linux.yml`, `macos.yml`, `windows.yml`, `android.yml`, `ios.yml`) share logic via composite actions and reusable workflows. Python helpers in `scripts/` are invoked by both.

## Layout

```
.github/
├── workflows/
│   ├── linux.yml, macos.yml, windows.yml        # Desktop build + test
│   ├── android.yml, ios.yml                     # Mobile builds
│   ├── _detect-changes.yml                      # Reusable: skip builds on unrelated PRs
│   ├── build-results.yml                        # Aggregate PR comment (workflow_run trigger)
│   ├── build-gstreamer.yml                      # GStreamer SDK builds
│   ├── build-profile.yml                        # CMake build profiling
│   ├── custom-build.yml                         # Custom build validation
│   ├── docker.yml                               # Docker image builds
│   ├── pre-commit.yml                           # Linting and formatting checks
│   ├── check-links.yml                          # Markdown link validation
│   ├── ci-scripts.yml                           # CI Python script tests
│   ├── analysis.yml                             # Static analysis
│   ├── pr-checks.yml                            # PR validation checks
│   ├── release.yml                              # Release automation
│   ├── docs.yml, doxygen.yml                    # Documentation deployment
│   ├── cache-cleanup.yml, cache-cleanup-pr.yml  # Cache maintenance
│   ├── crowdin.yml, lupdate.yml                 # Translation workflows
│   ├── dependency-review.yml                    # Dependency security review
│   ├── scorecard.yml                            # OpenSSF Scorecard
│   ├── flatpak.yml                              # Flatpak builds
│   ├── px4-metadata.yml                         # PX4 metadata sync
│   └── welcome.yml                              # New contributor welcome
├── actions/
│   ├── cmake-configure/                         # CMake configure with consistent options
│   ├── cmake-build/                             # Build with timing, reviewdog, ccache
│   ├── run-unit-tests/                          # CTest runner with JUnit output
│   ├── detect-changes/                          # Path-based change detection per platform
│   ├── attest-and-upload/                       # SBOM attestation + artifact upload
│   ├── attest-sbom/                             # SBOM generation and attestation
│   ├── aws-upload/                              # AWS S3 upload
│   ├── build-config/                            # Read build-config.json values
│   ├── build-prerequisites/                     # Install build prerequisites (GStreamer, etc.)
│   ├── build-setup/                             # Common build environment setup
│   ├── build-action/                            # Unified build action
│   ├── cache/                                   # Caching helpers
│   ├── cache-cleanup/                           # Delete branch / PR caches
│   ├── collect-artifact-sizes/                  # Artifact size collection
│   ├── coverage/                                # Code coverage reports
│   ├── custom-build/                            # Custom build support
│   ├── deploy-docs/                             # Deploy built docs to external repo
│   ├── docker/                                  # Docker build helpers
│   ├── download-all-artifacts/                  # Cross-workflow artifact download
│   ├── install-dependencies/                    # Platform dependency installation
│   ├── playstore/                               # Google Play Store upload
│   ├── qt-install/                              # Qt SDK installation with caching
│   ├── qt-android/, qt-ios/                     # Mobile Qt setup
│   ├── setup-python/                            # Python + uv + dependency installation
│   ├── size-analysis/                           # Binary size tracking
│   ├── test-duration-report/                    # Test timing analysis
│   ├── test-report/                             # Test result publishing
│   └── verify-executable/                       # Post-build executable verification
├── scripts/                                     # Python scripts for CI jobs
│   ├── templates/                               # Jinja2 templates (build_results.md.j2)
│   └── tests/                                   # Tests for CI scripts (pytest)
├── build-config.json                            # Centralized version numbers
└── build-config.schema.json                     # JSON Schema for build-config.json
```

## CI Conventions

- **Dependencies**: CI Python scripts use `httpx` for GitHub API access and `jinja2` for templating. Deps managed in `tools/pyproject.toml` under `[project.optional-dependencies] scripts`.
- **Shared helpers**: `gh_actions.py` provides GitHub API pagination (httpx) with `gh` CLI fallback. Import as `from common.gh_actions import ...`.
- **Bootstrap scripts** (`install_dependencies/` package, `ccache_helper.py`): Use stdlib only — they run before dependencies are installed.
- **Config**: Version numbers and build settings live in `.github/build-config.json`. Read via `common.build_config.get_build_config_value()`.
- **Outputs**: Use `common.gh_actions.write_github_output()` for `$GITHUB_OUTPUT` writes.

## Scripts

Python helpers in `.github/scripts/` invoked by workflows and composite actions.

| Script | Purpose |
|---|---|
| `android_boot_test.py` | Android emulator boot smoke test |
| `android_collect_diagnostics.py` | Collect emulator failure logs (build, adb dumps, GStreamer error grep, AVD logs) |
| `ccache_helper.py` | Ccache CI helper: config output, binary install, build summary |
| `check_baseline_ready.py` | Verify baseline-cache update readiness for platform workflows |
| `ci_bootstrap.py` | Bootstrap helper that makes `tools/common` imports work for CI scripts |
| `collect_artifact_sizes.py` | Collect artifact sizes for latest successful platform workflow runs |
| `collect_build_status.py` | Collect latest platform/pre-commit status for build-results comments |
| `coverage_comment.py` | Build coverage report comments |
| `cpm_helper.py` | CPM CI helper: dependency fingerprint, source cache configuration |
| `generate_build_results_comment.py` | Generate consolidated PR build-results comment |
| `gstreamer_archive.py` | Package GStreamer builds and optionally upload to S3 |
| `plan_docker_builds.py` | Generate Docker workflow build matrices from changed files |
| `precommit_results.py` | Normalize pre-commit outputs into uploaded CI artifacts |
| `size_analysis.py` | Analyze binary size changes |
| `test_duration_report.py` | Generate test-duration reports and regressions |

## Tests

Run the CI script tests locally:

```bash
pytest -q .github/scripts/tests/
```
