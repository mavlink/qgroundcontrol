#!/usr/bin/env bash
#
# Analyze binary size using bloaty and standard tools
#
# Usage:
#   size-analysis.sh --binary PATH [--output FILE] [--install-bloaty]
#
# Outputs (for GitHub Actions):
#   binary_size, stripped_size, symbol_count
#

set -euo pipefail

# Defaults
BINARY_PATH=""
OUTPUT_FILE="size-metrics.json"
INSTALL_BLOATY="false"
BLOATY_TIMEOUT=120

usage() {
    cat >&2 <<EOF
Usage: $(basename "$0") [OPTIONS]

Options:
  --binary PATH         Path to binary to analyze (required)
  --output FILE         Output JSON file (default: size-metrics.json)
  --install-bloaty      Install bloaty from source
  --bloaty-timeout SEC  Timeout for bloaty installation (default: 120)
  -h, --help            Show this help

Outputs (GITHUB_OUTPUT):
  binary_size    - Size of binary in bytes
  stripped_size  - Size of stripped binary in bytes
  symbol_count   - Number of symbols
EOF
    exit 1
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --binary) BINARY_PATH="$2"; shift 2 ;;
        --output) OUTPUT_FILE="$2"; shift 2 ;;
        --install-bloaty) INSTALL_BLOATY="true"; shift ;;
        --bloaty-timeout) BLOATY_TIMEOUT="$2"; shift 2 ;;
        -h|--help) usage ;;
        *) echo "Unknown option: $1" >&2; usage ;;
    esac
done

# Validate
[[ -z "$BINARY_PATH" ]] && { echo "Error: --binary required" >&2; usage; }
[[ ! -f "$BINARY_PATH" ]] && { echo "Error: Binary not found: $BINARY_PATH" >&2; exit 1; }

# Install bloaty if requested
if [[ "$INSTALL_BLOATY" == "true" ]] && ! command -v bloaty &> /dev/null; then
    echo "Installing bloaty (timeout: ${BLOATY_TIMEOUT}s)..."
    if timeout "$BLOATY_TIMEOUT" bash -c '
        sudo apt-get update
        sudo apt-get install -y libprotobuf-dev protobuf-compiler libre2-dev libcapstone-dev || true
        git clone --depth 1 https://github.com/google/bloaty.git /tmp/bloaty
        cd /tmp/bloaty
        cmake -B build -DCMAKE_BUILD_TYPE=Release -DBLOATY_ENABLE_RE2=ON
        cmake --build build --parallel
        sudo cmake --install build
    '; then
        echo "bloaty installed successfully"
    else
        echo "::warning::bloaty installation timed out, size analysis will be limited"
    fi
fi

echo "Analyzing: $BINARY_PATH"

# Binary size
binary_size=$(stat -c%s "$BINARY_PATH")
echo "Binary size: $binary_size bytes"

# Stripped size
stripped_binary="${BINARY_PATH}.stripped"
cp "$BINARY_PATH" "$stripped_binary"
strip "$stripped_binary"
stripped_size=$(stat -c%s "$stripped_binary")
rm -f "$stripped_binary"
echo "Stripped size: $stripped_size bytes"

# Symbol count
symbol_count=$(nm "$BINARY_PATH" 2>/dev/null | wc -l || echo "0")
echo "Symbol count: $symbol_count"

# Output for GitHub Actions
if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
    {
        echo "binary_size=$binary_size"
        echo "stripped_size=$stripped_size"
        echo "symbol_count=$symbol_count"
    } >> "$GITHUB_OUTPUT"
fi

# Generate metrics JSON
cat > "$OUTPUT_FILE" << EOF
[
  {
    "name": "Binary Size",
    "unit": "bytes",
    "value": $binary_size
  },
  {
    "name": "Stripped Size",
    "unit": "bytes",
    "value": $stripped_size
  },
  {
    "name": "Symbol Count",
    "unit": "symbols",
    "value": $symbol_count
  }
]
EOF

echo "Metrics written to: $OUTPUT_FILE"

# Write to step summary if available
if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
    # Section sizes
    echo "=== Section sizes ===" >> "$GITHUB_STEP_SUMMARY"
    echo '```' >> "$GITHUB_STEP_SUMMARY"
    size -A "$BINARY_PATH" >> "$GITHUB_STEP_SUMMARY"
    echo '```' >> "$GITHUB_STEP_SUMMARY"

    # Bloaty analysis if available
    if command -v bloaty &> /dev/null; then
        echo "" >> "$GITHUB_STEP_SUMMARY"
        echo "=== Top 20 largest symbols ===" >> "$GITHUB_STEP_SUMMARY"
        echo '```' >> "$GITHUB_STEP_SUMMARY"
        bloaty -d symbols -n 20 "$BINARY_PATH" >> "$GITHUB_STEP_SUMMARY" 2>/dev/null || echo "Bloaty analysis skipped"
        echo '```' >> "$GITHUB_STEP_SUMMARY"

        echo "" >> "$GITHUB_STEP_SUMMARY"
        echo "=== Size by compilation unit ===" >> "$GITHUB_STEP_SUMMARY"
        echo '```' >> "$GITHUB_STEP_SUMMARY"
        bloaty -d compileunits -n 20 "$BINARY_PATH" >> "$GITHUB_STEP_SUMMARY" 2>/dev/null || echo "Bloaty analysis skipped"
        echo '```' >> "$GITHUB_STEP_SUMMARY"
    fi

    # Summary table
    binary_mb=$(echo "scale=2; $binary_size / 1048576" | bc)
    stripped_mb=$(echo "scale=2; $stripped_size / 1048576" | bc)

    cat >> "$GITHUB_STEP_SUMMARY" << EOF

## Size Metrics

| Metric | Value |
|--------|-------|
| Binary Size | ${binary_mb} MB ($binary_size bytes) |
| Stripped Size | ${stripped_mb} MB ($stripped_size bytes) |
| Symbol Count | $symbol_count |
EOF
fi
