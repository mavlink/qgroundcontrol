#!/usr/bin/env bash
# Regenerates the encoded streams the GStreamer tests send, so the tests themselves need no encoder (x264enc and x265enc
# are GPL and CPU-heavy). The tests split the files into access units with h264parse/h265parse and pace them through
# appsrc (GStreamerTestHelpers.h, GstTest::FixtureRtpSender).
#
#   h264_320x240.h264  H.264 Annex B, 320x240, 30 fps, 60 frames, constrained baseline, IDR every 5 frames
#   h265_320x240.h265  H.265 Annex B, 320x240, 30 fps, 60 frames, main, IDR every 5 frames
#   h264_160x120.h264  H.264 Annex B, 160x120, 30 fps, 30 frames, constrained baseline, IDR every 5 frames
#
# Every IDR repeats the parameter sets, so any keyframe starts a stream; there are no B-frames, so PTS equals DTS.
# Needs gst-launch-1.0 with videotestsrc, x264enc, x265enc, h264parse and h265parse.
set -euo pipefail
cd "$(dirname "$0")"

source_caps() {
    echo "videotestsrc num-buffers=$3 pattern=ball ! video/x-raw,format=I420,width=$1,height=$2,framerate=30/1"
}

h264() {
    # shellcheck disable=SC2046 # the pipeline description is split into gst-launch arguments on purpose
    gst-launch-1.0 -q $(source_caps "$1" "$2" "$3") ! \
        x264enc tune=zerolatency speed-preset=ultrafast threads=1 key-int-max=5 bitrate=250 \
        option-string=repeat-headers=1 ! video/x-h264,profile=constrained-baseline ! \
        h264parse ! video/x-h264,stream-format=byte-stream,alignment=au ! filesink location="$4"
}

h265() {
    # shellcheck disable=SC2046
    gst-launch-1.0 -q $(source_caps "$1" "$2" "$3") ! \
        x265enc tune=zerolatency speed-preset=ultrafast key-int-max=5 bitrate=250 log-level=error \
        option-string=repeat-headers=1:open-gop=0:frame-threads=1:pools=none ! \
        h265parse ! video/x-h265,stream-format=byte-stream,alignment=au ! filesink location="$4"
}

h264 320 240 60 h264_320x240.h264
h265 320 240 60 h265_320x240.h265
h264 160 120 30 h264_160x120.h264
