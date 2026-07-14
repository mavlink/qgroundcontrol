#!/bin/bash -eu

custom_options="$SRC/qgroundcontrol/cmake/CustomOptions.cmake"
mavlink_dialect="$(sed -n 's/^set(QGC_MAVLINK_DIALECT "\([^"]*\)".*/\1/p' "$custom_options")"
mavlink_version="$(sed -n 's/^set(QGC_MAVLINK_VERSION "\([^"]*\)".*/\1/p' "$custom_options")"
generated_include="$WORK/mavlink-include"

test -n "$mavlink_dialect"
test -n "$mavlink_version"

PYTHONPATH="$SRC/mavlink" python3 -S -m pymavlink.tools.mavgen \
    --no-validate \
    --lang=C \
    --wire-protocol="$mavlink_version" \
    --output "$generated_include/mavlink" \
    "$SRC/mavlink/message_definitions/v1.0/${mavlink_dialect}.xml"

include_flags=(
    -I"$generated_include/mavlink"
    -I"$generated_include/mavlink/$mavlink_dialect"
)
read -r -a cxx_flags <<< "$CXXFLAGS"
read -r -a fuzzing_engine <<< "$LIB_FUZZING_ENGINE"
fuzzer_source="$SRC/qgroundcontrol/test/Fuzz/MAVLinkParserFuzzer.cc"
fuzzer_name="mavlink_parser_fuzzer"

"$CXX" "${cxx_flags[@]}" -std=c++20 "${include_flags[@]}" \
    "$fuzzer_source" "${fuzzing_engine[@]}" -o "$OUT/$fuzzer_name"

"$CXX" "${cxx_flags[@]}" -std=c++20 "${include_flags[@]}" \
    -DQGC_MAVLINK_SEED_GENERATOR "$fuzzer_source" -o "$WORK/mavlink_seed_generator"
mkdir -p "$WORK/mavlink-corpus"
"$WORK/mavlink_seed_generator" "$WORK/mavlink-corpus/heartbeat"
python3 - "$WORK/mavlink-corpus" "$OUT/${fuzzer_name}_seed_corpus.zip" <<'PY'
from pathlib import Path
import sys
import zipfile

corpus_dir = Path(sys.argv[1])
with zipfile.ZipFile(sys.argv[2], "w", compression=zipfile.ZIP_DEFLATED) as archive:
    for seed in corpus_dir.iterdir():
        archive.write(seed, arcname=seed.name)
PY

cp "$SRC/qgroundcontrol/test/Fuzz/MAVLinkParserFuzzer.dict" "$OUT/${fuzzer_name}.dict"
