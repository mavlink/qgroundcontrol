# GitHub Actions Scripts

Python helper scripts used by workflows and composite actions in this repository.

## Scripts

| Script | Purpose |
|---|---|
| `benchmark_runner.py` | Run benchmark binaries and summarize output |
| `coverage_comment.py` | Build coverage report comments |
| `find_binary.py` | Locate produced binaries/artifacts in build trees |
| `generate_build_results_comment.py` | Generate consolidated PR build-results comment |
| `generate_matrix.py` | Generate workflow matrix entries from `.github/build-config.json` |
| `gstreamer_archive.py` | Package GStreamer builds and optionally upload to S3 |
| `install_ccache.py` | Resolve ccache config and install pinned Linux binary |
| `size_analysis.py` | Analyze binary size changes |

## Tests

Run the CI script tests locally:

```bash
pytest -q \
  .github/scripts/tests/test_find_binary.py \
  .github/scripts/tests/test_generate_matrix.py \
  .github/scripts/tests/test_generate_build_results_comment.py \
  .github/scripts/tests/test_gstreamer_archive.py \
  .github/scripts/tests/test_install_ccache.py
```
