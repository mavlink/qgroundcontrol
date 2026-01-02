# GitHub Actions Scripts

Python scripts for CI/CD workflows.

## Scripts

| Script | Description |
|--------|-------------|
| `generate-matrix.py` | Generate build matrix from `build-config.json` |
| `install_ccache.py` | Install ccache with signature verification |
| `gstreamer_archive.py` | Create GStreamer archives and upload to S3 |
| `size_analysis.py` | Analyze binary size with bloaty |
| `benchmark-runner.py` | Run Qt test benchmarks |
| `coverage-comment.py` | Post coverage comments on PRs |
| `check-sizes.py` | Report artifact sizes |
| `find-binary.sh` | Locate QGroundControl binary in build dir |

## Usage

Scripts are called from composite actions in `.github/actions/`.

```bash
# Generate matrix for Linux
python3 .github/scripts/generate-matrix.py --platform linux

# Install ccache 4.12.2
python3 .github/scripts/install_ccache.py --version 4.12.2 --install

# Create GStreamer archive
python3 .github/scripts/gstreamer_archive.py --platform linux --arch x86_64 --version 1.24.0
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
