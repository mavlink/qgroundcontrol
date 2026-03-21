# GitHub Actions Scripts

Python helper scripts used by workflows and composite actions in this repository.

## Scripts

| Script | Purpose |
|---|---|
| `android_boot_test.py` | Android emulator boot smoke test |
| `benchmark_runner.py` | Run benchmark binaries and summarize output |
| `ccache_helper.py` | Ccache CI helper: config output, binary install, build summary |
| `check_baseline_ready.py` | Verify baseline-cache update readiness for platform workflows |
| `ci_bootstrap.py` | Bootstrap helper that makes `tools/common` imports work for CI scripts |
| `collect_artifact_sizes.py` | Collect artifact sizes for latest successful platform workflow runs |
| `collect_build_status.py` | Collect latest platform/pre-commit status for build-results comments |
| `coverage_comment.py` | Build coverage report comments |
| `find_binary.py` | Locate produced binaries/artifacts in build trees |
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
