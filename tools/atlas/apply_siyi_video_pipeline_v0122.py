#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re
import shutil
import sys

ROOT = Path(__file__).resolve().parents[2]
SOURCE_FACTORY = ROOT / "src/VideoManager/VideoReceiver/GStreamer/GstSourceFactory.cc"
VIDEO_RECEIVER = ROOT / "src/VideoManager/VideoReceiver/GStreamer/GstVideoReceiver.cc"
BACKUP_SUFFIX = ".pre-siyi-video-v0.12.2"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def backup(path: Path) -> None:
    backup_path = path.with_name(path.name + BACKUP_SUFFIX)
    if not backup_path.exists():
        shutil.copy2(path, backup_path)
        print(f"[backup] {backup_path.relative_to(ROOT)}")


def patch_source_factory() -> None:
    path = SOURCE_FACTORY
    require(path.exists(), f"Missing {path.relative_to(ROOT)}")
    text = path.read_text(encoding="utf-8")

    if "[ATLAS SIYI VIDEO] RTSP source: SIYI baseline" in text:
        print("[ok] GstSourceFactory.cc already uses the SIYI RTSP baseline")
        return

    backup(path)

    # Direct RTSP path copied semantically from SIYI-QGroundControl:
    #   rtspsrc location=<uri> latency=17 udp-reconnect=1 timeout=5000000
    #   protocols=0x00000004 (TCP interleaved only)
    # No ATLAS RTP retransmission/drop tuning is applied to RTSP.
    function_re = re.compile(
        r"GstElement\* buildRtspSource\(const QString& uri, const QUrl& sourceUrl, const Config& config, guint latencyMs\)\n"
        r"\{.*?\n\}\n\nGstElement\* buildTcpSource",
        re.S,
    )

    replacement = r'''GstElement* buildRtspSource(const QString& uri, const QUrl& sourceUrl,
                                [[maybe_unused]] const Config& config,
                                [[maybe_unused]] guint latencyMs)
{
    if (!GStreamer::isValidRtspUri(uri.toUtf8().constData())) {
        qCWarning(GstSourceFactoryLog) << "Invalid RTSP URI:" << QGCNetworkHelper::redactedUrlForLogging(sourceUrl);
        return nullptr;
    }

    GstElement* source = gst_element_factory_make("rtspsrc", "source");
    if (!source) {
        qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('rtspsrc') failed";
        return nullptr;
    }

    // SIYI-QGroundControl RTSP baseline. Keep these values literal on purpose.
    // protocols 0x00000004 == GST_RTSP_LOWER_TRANS_TCP.
    constexpr guint kSiyiRtspLatencyMs = 17;
    constexpr guint64 kSiyiRtspTimeoutUs = G_GUINT64_CONSTANT(5000000);
    constexpr GstRTSPLowerTrans kSiyiRtspProtocols = GST_RTSP_LOWER_TRANS_TCP;

    const QByteArray location = uri.toUtf8();
    g_object_set(source,
                 "location", location.constData(),
                 "latency", kSiyiRtspLatencyMs,
                 "udp-reconnect", TRUE,
                 "timeout", kSiyiRtspTimeoutUs,
                 "protocols", kSiyiRtspProtocols,
                 nullptr);

    qCInfo(GstSourceFactoryLog)
        << "[ATLAS SIYI VIDEO] RTSP source: SIYI baseline — TCP only, latency=17 ms, timeout=5 s";
    return source;
}

GstElement* buildTcpSource'''

    text, count = function_re.subn(replacement, text, count=1)
    require(count == 1, "Could not identify buildRtspSource() in GstSourceFactory.cc; no file was written")

    # SIYI uses a plain parsebin on RTSP. Keep newer ATLAS/QGC parser tuning only
    # on non-RTSP paths so Jetson/UDP/MPEG-TS behavior is otherwise untouched.
    old = '''        } else {
            // parsebin creates the codec parser only after it sees the stream caps. Configure that
            // parser as soon as it is autoplugged, before the pipeline reaches PLAYING.
            (void) g_signal_connect(parser, "deep-element-added", G_CALLBACK(configureAutopluggedParser), nullptr);
        }'''
    new = '''        } else if (!isRtsp) {
            // SIYI RTSP path: plain parsebin defaults. Keep ATLAS/QGC parser
            // tuning only for non-RTSP sources.
            (void) g_signal_connect(parser, "deep-element-added", G_CALLBACK(configureAutopluggedParser), nullptr);
        }'''
    require(old in text, "Could not identify parsebin tuning block in GstSourceFactory.cc")
    text = text.replace(old, new, 1)

    old = '''#if defined(QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER)
        if (!isUdpH265) {
            (void) g_signal_connect(parser, "autoplug-query", G_CALLBACK(filterParserCaps), nullptr);
        }
#endif'''
    new = '''#if defined(QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER)
        if (!isUdpH265 && !isRtsp) {
            // SIYI RTSP path leaves parsebin negotiation untouched.
            (void) g_signal_connect(parser, "autoplug-query", G_CALLBACK(filterParserCaps), nullptr);
        }
#endif'''
    require(old in text, "Could not identify legacy parsebin caps block in GstSourceFactory.cc")
    text = text.replace(old, new, 1)

    path.write_text(text, encoding="utf-8")
    print("[patched] src/VideoManager/VideoReceiver/GStreamer/GstSourceFactory.cc")


def patch_video_receiver() -> None:
    path = VIDEO_RECEIVER
    require(path.exists(), f"Missing {path.relative_to(ROOT)}")
    text = path.read_text(encoding="utf-8")

    if "[ATLAS SIYI VIDEO] default pre-decoder queue" in text:
        print("[ok] GstVideoReceiver.cc already uses the SIYI decoder queue baseline")
        return

    backup(path)

    # SIYI creates decoderQueue as a normal GStreamer queue and does not make
    # it leaky / two-buffer deep. Remove only the ATLAS pre-decoder tuning.
    leaky_re = re.compile(
        r"\n\s*// leaky=downstream \(2\) \+ tiny depth:.*?"
        r"g_object_set\(decoderQueue,\n"
        r"\s*\"leaky\", 2,\n"
        r"\s*\"max-size-buffers\", 2,\n"
        r"\s*\"max-size-bytes\", 0,\n"
        r"\s*\"max-size-time\", G_GUINT64_CONSTANT\(0\),\n"
        r"\s*nullptr\);\n",
        re.S,
    )
    replacement = '''\n        // [ATLAS SIYI VIDEO] default pre-decoder queue, matching SIYI-QGroundControl.\n'''
    text, count = leaky_re.subn(replacement, text, count=1)
    require(count == 1, "Could not identify the ATLAS leaky decoderQueue block; no file was written")

    # These two parts already match SIYI and are deliberately not rewritten.
    require('gst_element_factory_make("decodebin3", nullptr)' in text,
            "Expected SIYI-compatible decodebin3 decoder was not found")
    require('"sync", (_buffer >= 0)' in text,
            "Expected SIYI-compatible sink sync rule was not found")

    path.write_text(text, encoding="utf-8")
    print("[patched] src/VideoManager/VideoReceiver/GStreamer/GstVideoReceiver.cc")


def verify() -> None:
    sf = SOURCE_FACTORY.read_text(encoding="utf-8")
    vr = VIDEO_RECEIVER.read_text(encoding="utf-8")

    checks = [
        ("RTSP TCP only", "kSiyiRtspProtocols = GST_RTSP_LOWER_TRANS_TCP" in sf),
        ("RTSP latency 17 ms", "kSiyiRtspLatencyMs = 17" in sf),
        ("RTSP timeout 5 s", "kSiyiRtspTimeoutUs = G_GUINT64_CONSTANT(5000000)" in sf),
        ("RTSP udp-reconnect enabled", '"udp-reconnect", TRUE' in sf),
        ("No UDP|TCP RTSP fallback", "GST_RTSP_LOWER_TRANS_UDP | GST_RTSP_LOWER_TRANS_TCP" not in sf),
        ("No custom RTSP retransmission property", '"do-retransmission", doRetransmission' not in sf),
        ("No custom RTSP drop-on-latency property", '"drop-on-latency", dropOnLatency' not in sf),
        ("Plain parsebin on RTSP", "} else if (!isRtsp) {" in sf),
        ("No legacy caps forcing on RTSP", "if (!isUdpH265 && !isRtsp)" in sf),
        ("Default pre-decoder queue", '"leaky", 2' not in vr and '"max-size-buffers", 2' not in vr),
        ("decodebin3 decoder", 'gst_element_factory_make("decodebin3", nullptr)' in vr),
        ("SIYI sink sync rule", '"sync", (_buffer >= 0)' in vr),
    ]

    failed = [name for name, ok in checks if not ok]
    for name, ok in checks:
        print(f"[verify] {name}: {'PASS' if ok else 'FAIL'}")
    if failed:
        raise RuntimeError("SIYI pipeline verification failed: " + ", ".join(failed))
    print("\nATLAS Video Control v0.12.2 + SIYI video pipeline: PASS")


def main() -> int:
    try:
        patch_source_factory()
        patch_video_receiver()
        verify()
        print("\nNo ATLAS v0.12.2 payload/UI/KML/recording file was changed by this patch.")
        return 0
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
