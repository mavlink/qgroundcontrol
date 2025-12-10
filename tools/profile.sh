#!/usr/bin/env bash
# shellcheck disable=SC2012,SC2086,SC2155
# SC2086: EXTRA_ARGS intentionally uses word splitting for multiple arguments
# SC2155: Local variable declarations with command substitution are acceptable here
#
# Profile QGroundControl using various tools
#
# Usage:
#   ./tools/profile.sh                     # Run with perf (default)
#   ./tools/profile.sh --memcheck          # Check for memory leaks (valgrind)
#   ./tools/profile.sh --callgrind         # CPU profiling (valgrind)
#   ./tools/profile.sh --massif            # Heap profiling (valgrind)
#   ./tools/profile.sh --heaptrack         # Heap profiling (heaptrack)
#   ./tools/profile.sh --perf              # CPU profiling (perf)
#   ./tools/profile.sh --sanitize          # Build with sanitizers
#
# Requirements:
#   - valgrind (for memcheck, callgrind, massif)
#   - perf (for CPU profiling)
#   - heaptrack (optional, for heap profiling)
#   - kcachegrind (optional, for viewing callgrind output)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info()  { echo -e "${BLUE}[INFO]${NC} $*"; }
log_ok()    { echo -e "${GREEN}[OK]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# Defaults
BUILD_DIR="$REPO_ROOT/build"
OUTPUT_DIR="$REPO_ROOT/profile"
MODE="perf"
EXECUTABLE="$BUILD_DIR/QGroundControl"
EXTRA_ARGS=""

show_help() {
    head -18 "$0" | tail -16
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            ;;
        --memcheck)
            MODE="memcheck"
            shift
            ;;
        --callgrind)
            MODE="callgrind"
            shift
            ;;
        --massif)
            MODE="massif"
            shift
            ;;
        --heaptrack)
            MODE="heaptrack"
            shift
            ;;
        --perf)
            MODE="perf"
            shift
            ;;
        --sanitize)
            MODE="sanitize"
            shift
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            EXECUTABLE="$BUILD_DIR/QGroundControl"
            shift 2
            ;;
        --)
            shift
            EXTRA_ARGS="$*"
            break
            ;;
        *)
            EXTRA_ARGS="$*"
            break
            ;;
    esac
done

check_executable() {
    if [[ ! -x "$EXECUTABLE" ]]; then
        log_error "Executable not found: $EXECUTABLE"
        log_info "Build the project first: cmake --build build"
        exit 1
    fi
}

run_memcheck() {
    if ! command -v valgrind &> /dev/null; then
        log_error "valgrind not found. Install with: sudo apt install valgrind"
        exit 1
    fi

    mkdir -p "$OUTPUT_DIR"
    local output="$OUTPUT_DIR/memcheck-$(date +%Y%m%d-%H%M%S).log"

    log_info "Running memory leak check..."
    log_info "Output: $output"

    valgrind \
        --leak-check=full \
        --show-leak-kinds=all \
        --track-origins=yes \
        --verbose \
        --log-file="$output" \
        --suppressions="$REPO_ROOT/tools/valgrind.supp" 2>/dev/null || true \
        "$EXECUTABLE" $EXTRA_ARGS

    log_ok "Memcheck complete. Results: $output"

    # Show summary
    echo ""
    grep -E "(definitely|indirectly|possibly) lost:" "$output" || true
}

run_callgrind() {
    if ! command -v valgrind &> /dev/null; then
        log_error "valgrind not found. Install with: sudo apt install valgrind"
        exit 1
    fi

    mkdir -p "$OUTPUT_DIR"
    local output="$OUTPUT_DIR/callgrind-$(date +%Y%m%d-%H%M%S).out"

    log_info "Running CPU profiling with callgrind..."
    log_info "Output: $output"
    log_warn "This will be SLOW. Use for targeted profiling."

    valgrind \
        --tool=callgrind \
        --callgrind-out-file="$output" \
        --collect-jumps=yes \
        --collect-systime=yes \
        "$EXECUTABLE" $EXTRA_ARGS

    log_ok "Callgrind complete. Results: $output"

    if command -v kcachegrind &> /dev/null; then
        log_info "Opening with kcachegrind..."
        kcachegrind "$output" &
    else
        log_info "Install kcachegrind to visualize: sudo apt install kcachegrind"
    fi
}

run_massif() {
    if ! command -v valgrind &> /dev/null; then
        log_error "valgrind not found. Install with: sudo apt install valgrind"
        exit 1
    fi

    mkdir -p "$OUTPUT_DIR"
    local output="$OUTPUT_DIR/massif-$(date +%Y%m%d-%H%M%S).out"

    log_info "Running heap profiling with massif..."
    log_info "Output: $output"

    valgrind \
        --tool=massif \
        --massif-out-file="$output" \
        --detailed-freq=1 \
        "$EXECUTABLE" $EXTRA_ARGS

    log_ok "Massif complete. Results: $output"

    # Print summary
    if command -v ms_print &> /dev/null; then
        ms_print "$output" | head -50
    fi
}

run_heaptrack() {
    if ! command -v heaptrack &> /dev/null; then
        log_error "heaptrack not found. Install with: sudo apt install heaptrack"
        exit 1
    fi

    mkdir -p "$OUTPUT_DIR"

    log_info "Running heap profiling with heaptrack..."

    heaptrack -o "$OUTPUT_DIR/heaptrack" "$EXECUTABLE" $EXTRA_ARGS

    local latest=$(ls -t "$OUTPUT_DIR"/heaptrack.*.gz 2>/dev/null | head -1)
    if [[ -n "$latest" ]]; then
        log_ok "Heaptrack complete. Results: $latest"

        if command -v heaptrack_gui &> /dev/null; then
            log_info "Opening with heaptrack_gui..."
            heaptrack_gui "$latest" &
        else
            log_info "Install heaptrack-gui to visualize"
        fi
    fi
}

run_perf() {
    if ! command -v perf &> /dev/null; then
        log_error "perf not found. Install with: sudo apt install linux-tools-generic"
        exit 1
    fi

    mkdir -p "$OUTPUT_DIR"
    local output="$OUTPUT_DIR/perf-$(date +%Y%m%d-%H%M%S).data"

    log_info "Running CPU profiling with perf..."
    log_info "Output: $output"

    # Check if we have permissions
    if [[ ! -w /proc/sys/kernel/perf_event_paranoid ]]; then
        log_warn "May need root or: sudo sysctl kernel.perf_event_paranoid=-1"
    fi

    perf record \
        -g \
        --call-graph dwarf \
        -o "$output" \
        "$EXECUTABLE" $EXTRA_ARGS || true

    log_ok "Perf recording complete. Results: $output"

    # Generate report
    log_info "Generating report..."
    perf report -i "$output" --no-children --sort=dso,sym | head -50

    echo ""
    log_info "Interactive view: perf report -i $output"
    log_info "Flamegraph: perf script -i $output | stackcollapse-perf.pl | flamegraph.pl > flame.svg"
}

run_sanitize() {
    log_info "Building with sanitizers..."

    local sanitize_build="$REPO_ROOT/build-sanitize"

    cmake -B "$sanitize_build" -S "$REPO_ROOT" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
        -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
        -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
        -G Ninja

    cmake --build "$sanitize_build" --parallel

    log_ok "Sanitizer build complete"
    log_info "Run: $sanitize_build/QGroundControl"
    log_info "Errors will be reported at runtime"

    # Set sanitizer options
    export ASAN_OPTIONS="detect_leaks=1:halt_on_error=0:print_stats=1"
    export UBSAN_OPTIONS="print_stacktrace=1"

    "$sanitize_build/QGroundControl" $EXTRA_ARGS
}

# Main
cd "$REPO_ROOT"

case "$MODE" in
    memcheck)
        check_executable
        run_memcheck
        ;;
    callgrind)
        check_executable
        run_callgrind
        ;;
    massif)
        check_executable
        run_massif
        ;;
    heaptrack)
        check_executable
        run_heaptrack
        ;;
    perf)
        check_executable
        run_perf
        ;;
    sanitize)
        run_sanitize
        ;;
    *)
        log_error "Unknown mode: $MODE"
        exit 1
        ;;
esac
