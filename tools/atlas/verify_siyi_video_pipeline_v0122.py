#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sf_path = ROOT / "src/VideoManager/VideoReceiver/GStreamer/GstSourceFactory.cc"
vr_path = ROOT / "src/VideoManager/VideoReceiver/GStreamer/GstVideoReceiver.cc"

if not sf_path.exists() or not vr_path.exists():
    print("ERROR: core GStreamer source files not found. Extract this bundle into the ATLAS-QGroundControl repo first.", file=sys.stderr)
    raise SystemExit(1)

sf = sf_path.read_text(encoding="utf-8")
vr = vr_path.read_text(encoding="utf-8")
checks = [
    ("RTSP TCP only", "kSiyiRtspProtocols = GST_RTSP_LOWER_TRANS_TCP" in sf),
    ("RTSP latency = 17 ms", "kSiyiRtspLatencyMs = 17" in sf),
    ("RTSP timeout = 5 s", "kSiyiRtspTimeoutUs = G_GUINT64_CONSTANT(5000000)" in sf),
    ("RTSP udp-reconnect = true", '"udp-reconnect", TRUE' in sf),
    ("No UDP|TCP RTSP fallback", "GST_RTSP_LOWER_TRANS_UDP | GST_RTSP_LOWER_TRANS_TCP" not in sf),
    ("No RTSP retransmission tuning", '"do-retransmission", doRetransmission' not in sf),
    ("No RTSP drop-on-latency tuning", '"drop-on-latency", dropOnLatency' not in sf),
    ("RTSP parsebin left plain", "} else if (!isRtsp) {" in sf and "if (!isUdpH265 && !isRtsp)" in sf),
    ("Decoder queue not leaky", '"leaky", 2' not in vr and '"max-size-buffers", 2' not in vr),
    ("Decoder is decodebin3", 'gst_element_factory_make("decodebin3", nullptr)' in vr),
    ("Video sink timing matches SIYI", '"sync", (_buffer >= 0)' in vr),
]

ok_all = True
for name, ok in checks:
    print(f"{name}: {'PASS' if ok else 'FAIL'}")
    ok_all &= ok

print("\n" + ("ATLAS v0.12.2 SIYI pipeline: PASS" if ok_all else "ATLAS v0.12.2 SIYI pipeline: FAIL"))
raise SystemExit(0 if ok_all else 1)
