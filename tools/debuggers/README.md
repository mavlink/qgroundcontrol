# QGroundControl Debuggers and Profilers

Tools for debugging, profiling, and analyzing QGroundControl performance and memory usage.

## Overview

This directory provides debugging aids including profiling scripts, debugger visualizations for Qt types, and memory leak detection configuration.

## Quick Start

```bash
# CPU profiling with perf (fastest, modern Linux)
./tools/debuggers/profile.py

# Memory leak checking
./tools/debuggers/profile.py --memcheck

# Detailed heap profiling
./tools/debuggers/profile.py --heaptrack

# Call graph analysis (slow but comprehensive)
./tools/debuggers/profile.py --callgrind
```

## Scripts

### profile.py

Comprehensive profiling tool supporting multiple backends for CPU, memory, and call graph analysis.

**Usage:**

```bash
# CPU profiling (default - perf, modern Linux)
./tools/debuggers/profile.py

# Memory leak checking (full report)
./tools/debuggers/profile.py --memcheck

# CPU profiling with call graph (valgrind, slow)
./tools/debuggers/profile.py --callgrind

# Heap memory profiling (peak usage, allocations)
./tools/debuggers/profile.py --massif

# Heap profiling with GUI (heaptrack)
./tools/debuggers/profile.py --heaptrack

# Build with sanitizers (ASAN/UBSAN)
./tools/debuggers/profile.py --sanitize

# Specify build directory
./tools/debuggers/profile.py --build-dir /path/to/build

# Pass arguments to QGroundControl
./tools/debuggers/profile.py -- --unittest
```

**Backends:**

| Backend | Tool | Use Case | Speed | Platform |
|---------|------|----------|-------|----------|
| **perf** | Linux perf | CPU hotspots, call stacks | Fast | Linux only |
| **memcheck** | Valgrind | Memory leaks, invalid accesses | Slow (10-30x) | Linux/macOS |
| **callgrind** | Valgrind | Detailed CPU profiling, call graphs | Very slow (50-100x) | Linux/macOS |
| **massif** | Valgrind | Heap memory usage over time | Slow | Linux/macOS |
| **heaptrack** | Heaptrack | Peak heap, allocations, flamegraph | Moderate | Linux only |
| **sanitize** | AddressSanitizer | Runtime memory/UB detection | Moderate (2-3x) | Linux/macOS/Windows |

**Output Locations:**

- Results saved to `./profile/` directory
- Timestamped reports (e.g., `perf-20240115-143022.data`)
- Valgrind suppressions applied from `valgrind.supp`

**Requirements:**

```bash
# Linux (perf)
sudo apt install linux-tools-generic

# Memory profiling (all platforms)
sudo apt install valgrind              # Linux
brew install valgrind                  # macOS

# Heap profiling (Linux)
sudo apt install heaptrack heaptrack-gui

# Call graph visualization (optional)
sudo apt install kcachegrind           # Linux
brew install kcachegrind               # macOS
```

**Examples:**

```bash
# Check for memory leaks in specific scenario
./tools/debuggers/profile.py --memcheck -- --unittest

# Find CPU hotspots
./tools/debuggers/profile.py
perf report -i profile/perf-*.data

# Generate flamegraph
perf script -i profile/perf-*.data | \
  stackcollapse-perf.pl | \
  flamegraph.pl > flame.svg

# Analyze heap usage patterns
./tools/debuggers/profile.py --massif
ms_print profile/massif-*.out | head -100

# Interactive call graph (if kcachegrind installed)
./tools/debuggers/profile.py --callgrind
kcachegrind profile/callgrind-*.out
```

## GDB Pretty Printers

Pretty-print Qt types in GDB debugger.

### gdb-pretty-printers/qt6.py

Formats for common Qt 6 types:
- `QString` - Shows actual string content
- `QList<T>` - Shows container size and elements
- `QMap<K,V>` - Shows key-value pairs
- `QVector<T>` - Shows vector contents
- `QPoint` - Shows x,y coordinates
- `QRect` - Shows geometry

**Installation (GDB):**

```bash
# Add to ~/.gdbinit
python
import sys
sys.path.insert(0, '/path/to/qgroundcontrol/tools/debuggers/gdb-pretty-printers')
from qt6 import register_printers
register_printers(gdb)
end
```

Or symlink in GDB data directory:

```bash
mkdir -p ~/.gdb/printers
cp gdb-pretty-printers/qt6.py ~/.gdb/printers/
```

**Usage in GDB:**

```gdb
(gdb) print myString        # Automatically formatted
$1 = "Hello, World!"

(gdb) print myList          # Shows [item1, item2, item3]
$2 = {item1, item2, item3}

(gdb) print myPoint         # Shows Point(x=10, y=20)
$3 = Point(10, 20)
```

## Visualizers

### qt6.natvis

Visual Studio debugger visualizations for Qt 6 types (Windows/Visual Studio only).

**Installation:**

```bash
# Copy to Visual Studio visualizer directory
cp qt6.natvis \
  "%LocalAppData%\Microsoft\VisualStudio\17.0_xxxxxxxx\Components\Debugger\"

# Or for all users
cp qt6.natvis "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Packages\Debugger\Visualizers\"
```

**Usage in Visual Studio:**

- Hover over Qt variables in debugger
- Variables automatically formatted (QString shows content, QList shows items, etc.)
- Expand/collapse nodes in variable inspector

## Valgrind Suppressions

### valgrind.supp

Suppressions for known false positives and third-party library leaks:
- Qt library initialization leaks
- OpenGL driver false positives
- System library false positives

Used automatically by `profile.py --memcheck`.

**Adding Suppressions:**

1. Run memcheck and find false positive
2. Generate suppression: `valgrind --gen-suppressions=all --output-fd=2 -- ...`
3. Add to `valgrind.supp`
4. Verify with next memcheck run

**Manual Verification:**

```bash
# Check suppressions are valid
valgrind --leak-check=full \
  --suppressions=valgrind.supp \
  ./build/QGroundControl --unittest
```

## Cross-Platform Debugging

### Linux

```bash
# GDB with pretty printers
gdb ./build/QGroundControl
(gdb) break Vehicle::Vehicle
(gdb) run
(gdb) print vehicle    # Qt types pretty-printed
```

### macOS

```bash
# LLDB (Xcode's debugger)
lldb ./build/QGroundControl
(lldb) breakpoint set -n Vehicle::Vehicle
(lldb) run
(lldb) p vehicle       # Qt types format natively in modern Xcode
```

### Windows

```powershell
# Visual Studio debugger (automatic with qt6.natvis)
# No additional setup needed, types format in debugger
```

## Memory Profiling Workflow

1. **Build Debug**:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```

2. **Run Memory Check**:
   ```bash
   ./tools/debuggers/profile.py --memcheck
   ```

3. **Review Results**:
   ```bash
   cat profile/memcheck-*.log | grep -A 10 "LEAK SUMMARY"
   ```

4. **Investigate Leaks**:
   - Search log for `definitely lost:` leaks
   - Use stack trace to find source
   - Fix or add suppression if false positive

## CPU Profiling Workflow

1. **Run perf**:
   ```bash
   ./tools/debuggers/profile.py
   ```

2. **View Interactive Report**:
   ```bash
   perf report -i profile/perf-*.data
   ```

3. **Navigate**:
   - `u` - Up in call stack
   - `d` - Down in call stack
   - `/` - Search symbols
   - `q` - Quit

4. **Generate Flamegraph**:
   ```bash
   # Requires flamegraph.pl and stackcollapse-perf.pl
   perf script -i profile/perf-*.data | \
     stackcollapse-perf.pl | \
     flamegraph.pl --width=1200 > flame.svg
   ```

## Sanitizer Configuration

Build with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
./tools/debuggers/profile.py --sanitize

# Or manually
cmake -B build-san -S . \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
  -G Ninja

cmake --build build-san
./build-san/QGroundControl
```

**Output:**

- Memory errors printed to stderr with stack trace
- Program may exit on first error (controlled by `ASAN_OPTIONS`)

## Tips

- **First-time profiling?** Start with `perf` (Linux) for CPU hotspots
- **Memory issues?** Use `--memcheck` for comprehensive leak detection
- **Need exact call chains?** Use `--callgrind` (slow but accurate)
- **Windows/macOS?** Sanitizers often faster than Valgrind
- **Large applications?** Heaptrack GUI is best for memory visualization

## Limitations

| Tool | Limitation | Workaround |
|------|-----------|-----------|
| perf | Linux only | Use heaptrack on macOS, sanitizers on Windows |
| Valgrind | Very slow (10-100x) | Profile small components or short runs |
| GDB pretty printers | Manual setup | Use GDB init file or install globally |
| qt6.natvis | Visual Studio only | Use GDB on Linux, LLDB on macOS |

## See Also

- [Valgrind Documentation](https://valgrind.org/)
- [perf Documentation](https://perf.wiki.kernel.org/)
- [heaptrack](https://github.com/KDE/heaptrack)
- [Qt Debugging Documentation](https://doc.qt.io/qt-6/debug.html)
- [AddressSanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer)
