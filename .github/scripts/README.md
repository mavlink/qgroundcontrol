# GitHub Actions Scripts

Python scripts for CI/CD workflows.

## Scripts

| Script | Description |
|--------|-------------|
| `benchmark_runner.py` | Run Qt test benchmarks |
| `build_action.py` | Build GitHub Actions from source (for unreleased fixes) |
| `build_results.py` | Generate combined PR comment with test/coverage/size results |
| `check_sizes.py` | Report artifact sizes |
| `coverage_comment.py` | Post coverage comments on PRs |
| `find_binary.py` | Locate QGroundControl binary in build dir |
| `generate_matrix.py` | Generate build matrix from `build-config.json` |
| `gstreamer_archive.py` | Create GStreamer archives and upload to S3 |
| `install_ccache.py` | Install ccache with signature verification |
| `size_analysis.py` | Analyze binary size with bloaty |

## Usage

Scripts are called from composite actions in `.github/actions/`.

```bash
# Generate matrix for Linux
python3 .github/scripts/generate_matrix.py --platform linux

# Install ccache 4.12.2
python3 .github/scripts/install_ccache.py --version 4.12.2 --install

# Create GStreamer archive
python3 .github/scripts/gstreamer_archive.py --platform linux --arch x86_64 --version 1.24.0

# Build action from unreleased commit
python3 .github/scripts/build_action.py actions/checkout --ref fix-branch --output ./local-action
```

## Environment Variables

| Variable | Script | Description |
|----------|--------|-------------|
| `CCACHE_PREFIX` | `install_ccache.py` | Override install prefix (default: `/usr/local`) |
| `GSTREAMER_PREFIX` | `gstreamer_archive.py` | Override GStreamer prefix (all platforms) |
| `GSTREAMER_WIN_PREFIX` | `gstreamer_archive.py` | Windows-specific GStreamer prefix |
| `RUNNER_OS` | `gstreamer_archive.py` | Platform for AWS CLI installation |
| `GITHUB_OUTPUT` | all | GitHub Actions output file |
| `GITHUB_STEP_SUMMARY` | all | GitHub Actions summary file |

## Testing

```bash
cd .github/scripts
pytest tests/ -v
```

## Dependencies

Scripts use Python 3.9+ standard library only (no external dependencies required).
