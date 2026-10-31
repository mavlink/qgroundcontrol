#!/usr/bin/env bash
# Regenerates the encoded streams the GStreamer tests send, so the tests themselves need no encoder (x264enc and x265enc
# are GPL and CPU-heavy). The tests split the files into access units with h264parse/h265parse and pace them through
# appsrc (GStreamerTestHelpers.h, GstTest::FixtureRtpSender).
#
#   h264_320x240.h264  H.264 Annex B, 320x240, 30 fps, 60 frames, constrained baseline, IDR every 5 frames
#   h265_320x240.h265  H.265 Annex B, 320x240, 30 fps, 60 frames, main, IDR every 5 frames
#   h264_160x120.h264  H.264 Annex B, 160x120, 30 fps, 30 frames, constrained baseline, IDR every 5 frames
#
# h264_160x120.h264 and h265_320x240.h265 are also the samples GStreamer::selfTest() decodes and records in the app;
# NVIDIA's HEVC decoders reject frames under 144 pixels high, so its H.265 sample is the 320x240 one.
# Every IDR repeats the parameter sets, so any keyframe starts a stream; there are no B-frames, so PTS equals DTS.
# The streams carry no SEI: x264 and x265 write their name, version and options into one, and CI's artifact
# report fails a mobile binary, which embeds the self-test samples, that contains those GPL encoder markers.
# Needs gst-launch-1.0 with videotestsrc, x264enc, x265enc, h264parse and h265parse, and python3.
set -euo pipefail
cd "$(dirname "$0")"

# strip_sei <file> <h264|h265>: drop the SEI NAL units (H.264 type 6, H.265 types 39 and 40) from an Annex B stream.
strip_sei() {
    python3 - "$1" "$2" <<'EOF'
import re
import sys
from pathlib import Path

path, codec = Path(sys.argv[1]), sys.argv[2]
data = path.read_bytes()
# A NAL unit runs from its start code, with the leading zero byte of a 4-byte one, to the next start code.
starts = [m.start() for m in re.finditer(b"\x00\x00\x01", data)]
begins = [s - 1 if s > 0 and data[s - 1] == 0 else s for s in starts]
kept = bytearray(data[: begins[0]])
for begin, start, end in zip(begins, starts, [*begins[1:], len(data)]):
    header = data[start + 3]
    nal_type = header & 0x1F if codec == "h264" else (header >> 1) & 0x3F
    if nal_type not in ({6} if codec == "h264" else {39, 40}):
        kept += data[begin:end]
path.write_bytes(kept)
EOF
}

source_caps() {
    echo "videotestsrc num-buffers=$3 pattern=ball ! video/x-raw,format=I420,width=$1,height=$2,framerate=30/1"
}

h264() {
    # shellcheck disable=SC2046 # the pipeline description is split into gst-launch arguments on purpose
    gst-launch-1.0 -q $(source_caps "$1" "$2" "$3") ! \
        x264enc tune=zerolatency speed-preset=ultrafast threads=1 key-int-max=5 bitrate=250 \
        option-string=repeat-headers=1 ! video/x-h264,profile=constrained-baseline ! \
        h264parse ! video/x-h264,stream-format=byte-stream,alignment=au ! filesink location="$4"
    strip_sei "$4" h264
}

h265() {
    # shellcheck disable=SC2046
    gst-launch-1.0 -q $(source_caps "$1" "$2" "$3") ! \
        x265enc tune=zerolatency speed-preset=ultrafast key-int-max=5 bitrate=250 log-level=error \
        option-string=repeat-headers=1:open-gop=0:frame-threads=1:pools=none ! \
        h265parse ! video/x-h265,stream-format=byte-stream,alignment=au ! filesink location="$4"
    strip_sei "$4" h265
}

h264 320 240 60 h264_320x240.h264
h265 320 240 60 h265_320x240.h265
h264 160 120 30 h264_160x120.h264
