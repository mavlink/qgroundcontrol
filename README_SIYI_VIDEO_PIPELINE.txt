ATLAS VIDEO CONTROL v0.12.2 — FULL BUNDLE + SIYI VIDEO PIPELINE
===============================================================

BASE
----
This archive is built directly from the user's uploaded:
  ATLAS_Payload_Mode_v0_12_2(1).zip

All original v0.12.2 payload files are kept byte-for-byte unchanged. That means
screen recording, local video recording settings, SIYI A8 controls, KML layer
hierarchy/styles/altitudes/3D walls, map UI, Jetson/manual routing, toolbar and
all other payload features from that archive are preserved.

The only new files in this archive are the SIYI video-pipeline patch/verifier,
this README and a one-command installer.

VIDEO PIPELINE CHANGE
---------------------
Only the QGroundControl core live-video path is patched in-place after extraction.
For RTSP, the Direct A8 path follows SIYI-QGroundControl semantics:

  rtspsrc
    location = original RTSP URI
    latency = 17 ms
    udp-reconnect = true
    timeout = 5,000,000 us
    protocols = TCP interleaved only

  RTSP parser = plain parsebin defaults
  decoder queue = normal/default queue (NOT leaky, NOT limited to 2 buffers)
  decoder = decodebin3
  video sink sync = (_buffer >= 0)

Removed from the A8 RTSP path:
  - UDP|TCP transport fallback
  - ATLAS RTSP retransmission tuning
  - ATLAS drop-on-latency tuning
  - legacy AVC/HVC1 caps forcing on RTSP
  - pre-decoder leaky=downstream queue
  - max-size-buffers=2 before H.26x decode

Non-RTSP paths are deliberately left on the current ATLAS/QGC implementation.
This avoids losing Jetson/UDP/MPEG-TS functionality while making the A8 RTSP
behaviour match SIYI where the previous custom tuning caused corruption.

INSTALL
-------
From the ATLAS-QGroundControl repository:

  unzip -o ~/Downloads/ATLAS_Payload_Mode_v0_12_2_SIYI_VIDEO_FULL.zip -d ~/ATLAS/ATLAS-QGroundControl
  cd ~/ATLAS/ATLAS-QGroundControl
  ./install_v0_12_2_siyi_video_full.sh
  ./build/Release/ATLAS-QGroundControl

For the SIYI-equivalent test use:
  Force video decoder priority = Force software decoder
  Force CPU video path = OFF
  Low Latency Mode = OFF/default

Do not launch with NVIDIA PRIME override variables for this test.

ROLLBACK OF CORE VIDEO PATCH
----------------------------
The patch makes one-time backups next to the two modified core files using:
  .pre-siyi-video-v0.12.2

The payload/UI/KML/recording files from the uploaded v0.12.2 are not rewritten
by the video patch script.
