#!/usr/bin/env python3
"""Offline structural checks for ATLAS Video Control v0.12.2.

These checks catch bundle/layout/protocol regressions without requiring the full
Qt 6.11.1/QGroundControl build tree. A real Qt/C++/QML build is still required
on the target development machine.
"""

from __future__ import annotations

import ast
import py_compile
import re
import runpy
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

REQUIRED = [
    "custom/atlas/CMakeLists.txt",
    "custom/atlas/src/AtlasCustomPlugin.h",
    "custom/atlas/src/AtlasCustomPlugin.cc",
    "custom/atlas/src/AtlasJetsonClient.h",
    "custom/atlas/src/AtlasJetsonClient.cc",
    "custom/atlas/src/AtlasSiyiClient.h",
    "custom/atlas/src/AtlasSiyiClient.cc",
    "custom/atlas/src/AtlasPayloadRouter.h",
    "custom/atlas/src/AtlasPayloadRouter.cc",
    "custom/atlas/src/AtlasScreenRecorder.h",
    "custom/atlas/src/AtlasScreenRecorder.cc",
    "custom/atlas/src/AtlasKmlManager.h",
    "custom/atlas/src/AtlasKmlManager.cc",
    "custom/atlas/src/FlyView.qml",
    "custom/atlas/src/FlyViewCustomLayer.qml",
    "custom/atlas/src/CustomMapItems.qml",
    "custom/atlas/src/FlyViewToolBar.qml",
    "custom/atlas/src/FlyViewToolBarIndicators.qml",
    "src/Toolbar/FlyViewToolBar.qml",
    "src/Toolbar/FlyViewToolBarIndicators.qml",
    "tools/atlas/mock_jetson_server.py",
    "tools/atlas/mock_siyi_camera.py",
]


def fail(message: str) -> None:
    raise AssertionError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="utf-8")


def strip_c_like(text: str) -> str:
    """Remove strings/comments enough for delimiter balance checks."""
    out = []
    i = 0
    state = "code"
    quote = ""
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""
        if state == "code":
            if c == "/" and n == "/":
                state = "line_comment"; out.extend("  "); i += 2; continue
            if c == "/" and n == "*":
                state = "block_comment"; out.extend("  "); i += 2; continue
            if c in ('"', "'"):
                quote = c; state = "string"; out.append(" "); i += 1; continue
            out.append(c); i += 1; continue
        if state == "line_comment":
            if c == "\n":
                state = "code"; out.append("\n")
            else:
                out.append(" ")
            i += 1; continue
        if state == "block_comment":
            if c == "*" and n == "/":
                state = "code"; out.extend("  "); i += 2
            else:
                out.append("\n" if c == "\n" else " "); i += 1
            continue
        if state == "string":
            if c == "\\":
                out.extend("  "); i += min(2, len(text) - i); continue
            if c == quote:
                state = "code"
            out.append("\n" if c == "\n" else " "); i += 1
    return "".join(out)


def check_delimiters(path: Path) -> None:
    text = strip_c_like(path.read_text(encoding="utf-8"))
    pairs = {"}": "{", "]": "[", ")": "("}
    stack: list[tuple[str, int]] = []
    for idx, c in enumerate(text):
        if c in "{[(":
            stack.append((c, idx))
        elif c in "}])":
            if not stack or stack[-1][0] != pairs[c]:
                fail(f"Delimiter mismatch in {path.relative_to(ROOT)} near offset {idx}")
            stack.pop()
    if stack:
        fail(f"Unclosed delimiter {stack[-1][0]} in {path.relative_to(ROOT)}")


def qml_api_check() -> None:
    header = read("custom/atlas/src/AtlasPayloadRouter.h")
    properties = set(re.findall(r"Q_PROPERTY\([^\n]*?\s([A-Za-z_]\w*)\s+(?:NOTIFY|CONSTANT|READ)", header))
    # More reliable extraction: name is the token immediately before READ.
    properties.update(re.findall(r"Q_PROPERTY\([^\n]*?\s([A-Za-z_]\w*)\s+READ\s", header))
    methods = set(re.findall(r"Q_INVOKABLE\s+[\w:<>&*\s]+?\s+([A-Za-z_]\w*)\s*\(", header))
    allowed = properties | methods

    qml_files = list((ROOT / "custom/atlas/src").glob("*.qml")) + list((ROOT / "src/Toolbar").glob("*.qml"))
    references: set[str] = set()
    for path in qml_files:
        text = path.read_text(encoding="utf-8")
        references.update(re.findall(r"\batlasPayload\.([A-Za-z_]\w*)", text))
        require("atlasJetsonClient" not in text, f"Direct atlasJetsonClient reference left in {path.relative_to(ROOT)}")

    unknown = sorted(references - allowed)
    require(not unknown, f"QML references missing from AtlasPayloadRouter API: {unknown}")


def check_siyi_protocol() -> None:
    module = runpy.run_path(str(ROOT / "tools/atlas/mock_siyi_camera.py"), run_name="mock_siyi_verify")
    crc = module["crc16_xmodem"]
    build = module["build_frame"]
    parse = module["parse_frame"]

    # Official SIYI SDK CRC example: header+payload -> CRC16/XMODEM 0x648D,
    # transmitted little-endian as 8D 64.
    official_vectors = {
        "55 66 01 01 00 00 00 05 01": 0x648D,  # zoom +1 -> 8D 64
        "55 66 01 01 00 00 00 05 FF": 0x6A5C,  # zoom -1 -> 5C 6A
        "55 66 01 00 00 00 00 18": 0x477C,     # current zoom -> 7C 47
        # Official codec example: MAIN H265, 1280x720, 1500 kbps -> 58 45
        "55 66 01 09 00 00 00 21 01 02 00 05 D0 02 DC 05 00": 0x4558,
    }
    for hex_bytes, expected_crc in official_vectors.items():
        require(crc(bytes.fromhex(hex_bytes)) == expected_crc,
                f"SIYI CRC16/XMODEM official vector failed: {hex_bytes}")

    for cmd, payload in [
        (0x05, b"\x01"),
        (0x07, bytes([25, 0xF0])),
        (0x0C, b"\x02"),
        (0x0E, bytes.fromhex("84 03 3E FE")),
        (0x18, b""),
        (0x20, b"\x01"),
        (0x21, bytes.fromhex("01 02 80 07 38 04 70 17 00")),
    ]:
        frame = build(cmd, payload, seq=42, control=1)
        control, seq, parsed_cmd, parsed_payload = parse(frame)
        require(control == 1 and seq == 42 and parsed_cmd == cmd and parsed_payload == payload,
                f"SIYI round-trip failed for cmd 0x{cmd:02X}")


def main() -> None:
    missing = [rel for rel in REQUIRED if not (ROOT / rel).is_file()]
    require(not missing, f"Missing required files: {missing}")
    print("ATLAS Video Control v0.12.2: BASIC CHECKS PASS")

    for path in list((ROOT / "custom/atlas/src").glob("*.qml")) + list((ROOT / "src/Toolbar").glob("*.qml")):
        check_delimiters(path)
    for path in list((ROOT / "custom/atlas/src").glob("*.h")) + list((ROOT / "custom/atlas/src").glob("*.cc")):
        check_delimiters(path)
    print("QML/C++ delimiters: PASS")

    for rel in ["tools/atlas/mock_jetson_server.py", "tools/atlas/mock_siyi_camera.py", "tools/atlas/verify_payload_bundle.py"]:
        py_compile.compile(str(ROOT / rel), doraise=True)
    print("Mock/verifier Python syntax: PASS")

    for svg in (ROOT / "custom/atlas/res").rglob("*.svg"):
        ET.parse(svg)
    print("SVG XML parse: PASS")

    qml_api_check()
    print("AtlasPayloadRouter QML API references: PASS")

    # Router-to-backend method names must exist in the concrete client headers.
    router_cc_api = read("custom/atlas/src/AtlasPayloadRouter.cc")
    for object_name, header_rel in [("_jetson", "custom/atlas/src/AtlasJetsonClient.h"),
                                    ("_siyi", "custom/atlas/src/AtlasSiyiClient.h")]:
        calls = set(re.findall(re.escape(object_name) + r"->([A-Za-z_]\w*)\s*\(", router_cc_api))
        backend_h = read(header_rel)
        declared = set(re.findall(r"\b([A-Za-z_]\w*)\s*\([^;{}]*\)\s*(?:const\s*)?(?:\{|;)", backend_h))
        missing_calls = sorted(calls - declared)
        require(not missing_calls, f"{object_name} backend methods missing: {missing_calls}")
    print("Payload router backend API references: PASS")

    # Catch local variables that hide zero-argument Qt signals. Besides being
    # confusing, code such as `const bool endpointChanged = ...; emit endpointChanged();`
    # is a hard C++ compile error because the local variable hides the signal method.
    for header_rel, source_rel in [
        ("custom/atlas/src/AtlasSiyiClient.h", "custom/atlas/src/AtlasSiyiClient.cc"),
        ("custom/atlas/src/AtlasJetsonClient.h", "custom/atlas/src/AtlasJetsonClient.cc"),
        ("custom/atlas/src/AtlasPayloadRouter.h", "custom/atlas/src/AtlasPayloadRouter.cc"),
        ("custom/atlas/src/AtlasScreenRecorder.h", "custom/atlas/src/AtlasScreenRecorder.cc"),
        ("custom/atlas/src/AtlasKmlManager.h", "custom/atlas/src/AtlasKmlManager.cc"),
    ]:
        header_text = read(header_rel)
        source_text = read(source_rel)
        signal_names = set(re.findall(r"\bvoid\s+([A-Za-z_]\w*Changed)\s*\(\s*\)\s*;", header_text))
        for signal_name in signal_names:
            hidden = re.search(rf"\b(?:const\s+)?(?:bool|int|double|QString|auto)\s+{re.escape(signal_name)}\b", source_text)
            require(not hidden, f"Local variable hides Qt signal {signal_name} in {source_rel}")
    print("Qt signal/local-name collision check: PASS")

    cmake = read("custom/atlas/CMakeLists.txt")
    for required_source in ["AtlasJetsonClient", "AtlasSiyiClient", "AtlasPayloadRouter", "AtlasScreenRecorder", "AtlasKmlManager", "AtlasCustomPlugin"]:
        require(f"src/{required_source}.cc" in cmake and f"src/{required_source}.h" in cmake,
                f"CMake missing {required_source}")
    print("CMake source registration: PASS")

    toolbar = read("custom/atlas/src/FlyViewToolBar.qml")
    layer = read("custom/atlas/src/FlyViewCustomLayer.qml")
    indicators = read("custom/atlas/src/FlyViewToolBarIndicators.qml")
    flyview = read("custom/atlas/src/FlyView.qml")
    router_h = read("custom/atlas/src/AtlasPayloadRouter.h")
    router_cc = read("custom/atlas/src/AtlasPayloadRouter.cc")
    siyi_cc = read("custom/atlas/src/AtlasSiyiClient.cc")
    plugin_cc = read("custom/atlas/src/AtlasCustomPlugin.cc")

    # Preserve the v0.9 map implementation the user already validated.
    for marker in ["AtlasMiniMap3D", "AtlasFullMap3D", "GeoMapCamera.Mode3D", "setAtlasMap3D", "CENTER UAV"]:
        require(marker in flyview, f"v0.9 2D/3D map invariant missing: {marker}")
    print("v0.9 2D/3D map baseline preserved: PASS")

    # v0.12.2 manual routing invariants.
    for marker in ["DIRECT A8", "JETSON", "Manual only", 'atlasPayload.setMode("DIRECT_A8")', 'atlasPayload.setMode("JETSON")']:
        require(marker in toolbar, f"Manual route UI invariant missing: {marker}")
    require("no autodetection and no automatic fallback" in router_cc,
            "Router must explicitly remain manual-only")
    require("_mode = Mode::Jetson" in router_h,
            "Expected backward-compatible first-run Jetson default missing")
    require('QStringLiteral("192.168.144.25")' in router_h and "37260" in router_h,
            "Direct A8 endpoint defaults missing")
    require('rtsp://192.168.144.25:8554/main.264' in router_h,
            "Known-good Direct A8 RTSP default missing")
    require('QStringLiteral("127.0.0.1")' in router_h and "45900" in router_h,
            "Jetson dev/mock control endpoint defaults missing")
    require("jetsonRtspUrl" in router_h and "directRtspUrl" in router_h,
            "Separate Direct/Jetson RTSP profiles missing")
    require("configureDirectA8" in toolbar and "configureJetson" in toolbar,
            "Separate endpoint editors missing")
    print("Manual dual-route + separate endpoint profiles: PASS")

    # Actual Direct A8 backend, not a UI-only mock.
    for marker in ["QUdpSocket", "_crc16Xmodem", "_sendCommand(0x07", "_sendCommand(0x05", "_sendCommand(0x0C", "_sendCommand(0x0D", "_sendCommand(0x18"]:
        require(marker in (read("custom/atlas/src/AtlasSiyiClient.h") + siyi_cc), f"Direct SIYI backend invariant missing: {marker}")
    check_siyi_protocol()
    print("SIYI framing/CRC vectors: PASS")

    # Direct mode degrades UI honestly; no fake RF/AI values.
    for marker in ["BYPASSED", "IP PATH OK", "N/A"]:
        require(marker in indicators, f"Direct toolbar status invariant missing: {marker}")
    for marker in ["RF stats unavailable", "atlasPayload.aiAvailable", "effectiveRightPanelWidth"]:
        require(marker in layer, f"Direct payload UI invariant missing: {marker}")
    require('visible: atlasPayload.aiAvailable' in layer, "AI panel must hide in Direct A8 mode")
    require('(atlasPayload.aiAvailable ? 780 : 463)' in layer and '(QGroundControl.videoManager.recording ? 18 : 0)' in layer and '(atlasScreenRecorder.recording || atlasScreenRecorder.starting)' in layer,
            "Direct RAW + recording compact video selector missing")
    print("Direct A8 UI availability/status semantics: PASS")

    # Three independent recording paths: payload-camera REC, local stream REC, and
    # local ATLAS-window screen REC. Screen capture must never depend on payload mode.
    screen_h = read("custom/atlas/src/AtlasScreenRecorder.h")
    screen_cc = read("custom/atlas/src/AtlasScreenRecorder.cc")
    for marker in ["QVideoFrameInput", "QMediaRecorder", "QMediaCaptureSession", "QQuickWindow", "startRecording", "stopRecording", "elapsedSeconds", "lastFile"]:
        require(marker in (screen_h + screen_cc), f"Screen recorder invariant missing: {marker}")
    for marker in ["grabWindow()", "_findAtlasWindow", "ATLAS", "QGroundControl", "QStandardPaths::MoviesLocation", "ATLAS_Screen_%1.mp4", "frameRate", "videoBitRateMbps", "resolutionMode", "effectiveFps"]:
        require(marker in screen_cc or marker in screen_h, f"ATLAS scene capture invariant missing: {marker}")
    require("QWindowCapture" not in (screen_h + screen_cc),
            "SCREEN REC must not depend on desktop QWindowCapture in v0.12.2")
    require('QStringLiteral("atlasScreenRecorder")' in plugin_cc and '&_screenRecorder' in plugin_cc,
            "atlasScreenRecorder context property missing")
    require('label: QGroundControl.videoManager.recording' in layer and '"VIDEO REC"' in layer,
            "VIDEO REC control missing")
    require('QGroundControl.videoManager.startRecording("")' in layer and 'QGroundControl.videoManager.stopRecording()' in layer,
            "VIDEO REC is not wired to QGC VideoManager")
    require('"SCREEN REC"' in layer and 'atlasScreenRecorder.toggleRecording()' in layer,
            "SCREEN REC control missing")
    require('selectedColor: "#8E1C28"' in layer and 'selectedBorderColor: root.dangerColor' in layer,
            "Recording active-state styling missing")
    require('running: QGroundControl.videoManager.recording' in layer,
            "VIDEO REC elapsed timer missing")
    require('QGroundControl.videoManager.decoding ? "VIDEO REC" : "VIDEO WAIT"' in layer,
            "VIDEO REC must distinguish configured source from a live decoded stream")
    require('videoRecordingPending' in layer and 'VIDEO REC did not start' in layer,
            "VIDEO REC pending/failure feedback missing")
    require('Q_PROPERTY(bool starting READ starting NOTIFY startingChanged)' in screen_h,
            "SCREEN REC starting state missing")
    require('recorderStateChanged' in screen_cc and 'RecordingState' in screen_cc,
            "SCREEN REC must wait for recorder confirmation before reporting active")
    require('[ATLAS SCREEN REC]' in screen_cc,
            "SCREEN REC terminal diagnostics missing")
    require('format.setVideoCodec(QMediaFormat::VideoCodec::MPEG4)' in screen_cc,
            "SCREEN REC must explicitly request the software-compatible MPEG-4 encoder")
    require('format.resolveForEncoding' not in screen_cc,
            "SCREEN REC must not let Qt auto-select HEVC/VAAPI again")
    require('_recordingFailed' in screen_h and 'Failed recording removed' in screen_cc,
            "SCREEN REC must not report a failed encoder session as a saved recording")
    print("Video/Screen/A8 recording separation + robust state feedback: PASS")

    # v0.12.2 configurable recording settings.
    for marker in [
        'Q_PROPERTY(int frameRate', 'Q_PROPERTY(int videoBitRateMbps',
        'Q_PROPERTY(QString resolutionMode', 'Q_PROPERTY(double effectiveFps',
        'settings.value(QStringLiteral("fps"), 60)', '_frameTimer.setInterval(1)',
        '_capturePeriodNs', 'Qt::KeepAspectRatio'
    ]:
        require(marker in (screen_h + screen_cc), f"Screen recording settings invariant missing: {marker}")
    for marker in ['MEDIA & CAMERA SETTINGS', '[20, 30, 60, 120, 144]', 'VIDEO RECORDING (LOCAL STREAM)',
                   'recordingFormat.rawValue', 'enableStorageLimit.rawValue', 'maxVideoSize.rawValue']:
        require(marker in layer, f"Recording settings UI invariant missing: {marker}")
    print("v0.12.2 screen/video recording settings: PASS")

    # Verified SIYI camera settings subset. FPS is read from 0x20; 0x21 uses a reserved byte.
    siyi_h = read("custom/atlas/src/AtlasSiyiClient.h")
    for marker in ['requestCodecSpecs', 'configureMainStream', 'configureRecordStream',
                   '_sendCommand(0x20', '_sendCommand(0x21', 'payload.append(static_cast<char>(0)); // reserved',
                   'setGimbalWorkingMode', 'setVideoOutput']:
        require(marker in (siyi_h + siyi_cc), f"SIYI camera settings invariant missing: {marker}")
    for marker in ['a8MainStreamFps', 'a8RecordStreamFps', 'refreshA8CodecSpecs',
                   'configureA8MainStream', 'configureA8RecordStream', 'setA8GimbalWorkingMode', 'setA8VideoOutput']:
        require(marker in router_h, f"SIYI router setting missing: {marker}")
    require('2K/4K/Boot-Record packets' in layer and 'does not expose an FPS field' in layer,
            "UI must state the verified SIYI SDK boundary instead of pretending undocumented settings are supported")
    require('recording-stream codec is not currently writable' in layer and
            'preserve codec; SDK says recording codec is not writable' in siyi_cc,
            "A8 recording-stream codec must be preserved per SDK V0.1.1")
    require('configureA8RecordStream(int width, int height, int bitrateKbps)' in router_h and
            '_siyi->configureRecordStream(width, height, bitrateKbps)' in router_cc and
            'configureRecordStream(int width, int height, int bitrateKbps)' in siyi_h,
            "A8 recording-stream API must not expose a writable codec selector")
    print("Verified SIYI A8 codec/gimbal/output settings: PASS")

    # KML import fidelity: hierarchy, styles, all common geometry types and
    # exact KML altitude semantics (no invented fallback height).
    kml_h = read("custom/atlas/src/AtlasKmlManager.h")
    kml_cc = read("custom/atlas/src/AtlasKmlManager.cc")
    wall_h = read("custom/atlas/src/AtlasKmlWallGeometry.h")
    wall_cc = read("custom/atlas/src/AtlasKmlWallGeometry.cc")

    for marker in [
        'Q_PROPERTY(QVariantList polygons',
        'Q_PROPERTY(QVariantList lines',
        'Q_PROPERTY(QVariantList points',
        'Q_PROPERTY(QVariantList treeItems',
        'setTreeItemVisible',
        'setTreeItemExpanded',
        'StyleMap',
        'LineString',
        'Point',
        '_kmlColorToQt',
        'relativeToGround',
        'absolute',
    ]:
        require(marker in (kml_h + kml_cc), f"KML fidelity invariant missing: {marker}")

    require('defaultHeightMeters' not in (kml_h + kml_cc),
            "KML renderer must not invent a fallback 3D height")
    require('extrudeFlatZones' not in (kml_h + kml_cc),
            "KML renderer must obey the file extrusion flag instead of forcing flat zones into volumes")

    for marker in [
        'import ATLAS.Kml 1.0',
        'atlasKml.polygons',
        'atlasKml.lines',
        'atlasKml.points',
        'atlasKml.treeItems',
        'KML / AIRSPACE LAYERS',
        'setTreeItemVisible',
        'setTreeItemExpanded',
        'AtlasKmlWallGeometry',
        'feature.fillEnabled',
        'feature.outlineEnabled',
        'feature.lineColor',
        'feature.altitudeModeCode',
    ]:
        require(marker in flyview, f"KML map UI invariant missing: {marker}")

    require('qmlRegisterType<AtlasKmlWallGeometry>' in plugin_cc,
            "AtlasKmlWallGeometry QML type is not registered")
    require('AtlasKmlWallGeometry.cc' in read("custom/atlas/CMakeLists.txt"),
            "AtlasKmlWallGeometry is not registered in CMake")
    require('QStringLiteral("atlasKml")' in plugin_cc and '&_kmlManager' in plugin_cc,
            "atlasKml context property missing")
    require('return altitude;' in wall_cc and 'return terrainHeight + altitude;' in wall_cc,
            "KML wall geometry must distinguish absolute AMSL from relative-to-ground AGL")
    print("KML Google-Earth-style hierarchy/style/altitude fidelity: PASS")
    # v0.12.2 Qt 6.11 AUTOMOC regression guards.
    wall_h = read("custom/atlas/src/AtlasKmlWallGeometry.h")
    manager_h = read("custom/atlas/src/AtlasKmlManager.h")
    require('Q_MOC_INCLUDE("GeoScene.h")' in wall_h,
            "AtlasKmlWallGeometry must expose GeoScene.h to Qt moc")
    require('Q_MOC_INCLUDE("SurfacePatchModel.h")' in wall_h,
            "AtlasKmlWallGeometry must expose SurfacePatchModel.h to Qt moc")
    require('NOTIFY zonesChanged' not in manager_h and 'NOTIFY visualizationSettingsChanged' not in manager_h,
            "AtlasKmlManager source header must not use removed legacy notify signals")
    require('READ zones' not in manager_h and 'READ extrudeFlatZones' not in manager_h and 'READ defaultHeightMeters' not in manager_h,
            "AtlasKmlManager source header must not expose removed v0.12 legacy accessors")
    print("Qt 6.11 AUTOMOC KML regression guards: PASS")


    # Router is the QML facade; low-level clients remain diagnostic only.
    require('QStringLiteral("atlasPayload")' in plugin_cc and '&_payloadRouter' in plugin_cc,
            "atlasPayload context property missing")
    require("_payloadRouter.start()" in plugin_cc, "Payload router is not started by custom plugin")
    print("Plugin/router wiring: PASS")

    # Core toolbar fallbacks must be identical to custom resource copies.
    require((ROOT / "custom/atlas/src/FlyViewToolBar.qml").read_bytes() == (ROOT / "src/Toolbar/FlyViewToolBar.qml").read_bytes(),
            "Toolbar fallback copy differs from custom override")
    require((ROOT / "custom/atlas/src/FlyViewToolBarIndicators.qml").read_bytes() == (ROOT / "src/Toolbar/FlyViewToolBarIndicators.qml").read_bytes(),
            "Toolbar indicators fallback copy differs from custom override")
    print("Toolbar fallback copies synchronized: PASS")

    # Detect accidental transcript/shell wrapper contamination in source files.
    forbidden = ["```", "<assistant", "<user", "Tool uploaded file"]
    for path in list((ROOT / "custom/atlas").rglob("*")) + list((ROOT / "src/Toolbar").glob("*")):
        if path.is_file() and path.suffix in {".qml", ".cc", ".h", ".txt", ".cmake"}:
            text = path.read_text(encoding="utf-8", errors="ignore")
            for token in forbidden:
                require(token not in text, f"Wrapper contamination {token!r} in {path.relative_to(ROOT)}")
    print("No wrapper contamination: PASS")

    print("v0.12.2 requested invariants: PASS")
    print("Final Qt/C++/QML compilation must still be run with the project's Qt 6.11.1 toolchain.")


if __name__ == "__main__":
    main()
