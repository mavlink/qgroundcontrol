#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

echo "== 1/5 Verify original ATLAS v0.12.2 payload bundle =="
python3 tools/atlas/verify_payload_bundle.py

echo "== 2/5 Apply SIYI-QGroundControl video pipeline semantics =="
python3 tools/atlas/apply_siyi_video_pipeline_v0122.py

echo "== 3/5 Verify SIYI video pipeline =="
python3 tools/atlas/verify_siyi_video_pipeline_v0122.py

echo "== 4/5 Reconfigure Qt/CMake =="
rm -rf build/ATLAS-QGroundControl_autogen build/CMakeFiles/ATLAS-QGroundControl_autogen.dir
PATH="$HOME/Qt/Tools/CMake/bin:$PATH" "$HOME/Qt/6.11.1/gcc_64/bin/qt-cmake" -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

echo "== 5/5 Build =="
"$HOME/Qt/Tools/CMake/bin/cmake" --build build --parallel 2

echo
echo "PASS: ATLAS v0.12.2 preserved + SIYI video pipeline applied."
echo "Run normally with: ./build/Release/ATLAS-QGroundControl"
