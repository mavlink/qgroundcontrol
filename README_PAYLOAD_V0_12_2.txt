ATLAS VIDEO CONTROL v0.12.2
=========================

Built directly on the validated v0.11.3 baseline.

WHAT IS NEW
-----------

1) Screen recording settings
   - Default: 60 fps (previously fixed at 20 fps).
   - Selectable: 20 / 30 / 60 / 120 / 144 fps.
   - Resolution: Native / 1080p / 720p.
   - Bitrate: 8 / 12 / 16 / 20 / 40 / 60 Mbps from the UI.
   - Settings persist in QSettings.
   - Effective capture FPS is reported while recording. This is important at
     120/144 fps because QQuickWindow::grabWindow() readback may be the real
     bottleneck even if 144 fps is requested.
   - The screen recorder remains fully independent from RTSP/video and keeps
     recording the ATLAS UI when payload video is lost.
   - Keeps the v0.11.3 MPEG-4 software encoding path that worked on the
     operator Linux machine (avoids the failing HEVC/VAAPI auto-selection).

2) Local VIDEO REC settings
   - File container: MP4 / MOV / MKV using the native QGC VideoManager setting.
   - Enable/disable automatic storage limiting.
   - Configure maximum recording storage in MB.
   - Displays the actual QGC video save directory.
   - VIDEO REC intentionally preserves the source stream FPS/bitrate rather
     than manufacturing duplicated frames.

3) SIYI A8 camera settings (verified public SDK subset)
   - Requests real main-stream and recording-stream codec status using 0x20.
   - Displays codec, resolution, bitrate and camera-reported FPS.
   - Main stream: writes H264/H265, 720p/1080p and bitrate using 0x21.
   - Recording stream: writes 720p/1080p and bitrate while preserving the codec
     reported by the camera; A8 SDK V0.1.1 says recording codec changes are not
     currently supported.
   - Gimbal working mode: FOLLOW / LOCK / FPV.
   - Video output: ETHERNET / HDMI / CVBS (camera restart may be required).
   - FPS is read-only because the documented A8 0x21 write packet ends in a
     reserved byte rather than a frame-rate field.
   - UniGCS exposes more A8 settings (including higher recording resolutions,
     Boot Record, ISO/shutter/white-balance). v0.12.2 deliberately does not send
     guessed/undocumented A8 Ethernet commands for those controls.

4) KML flight/restricted-zone overlay
   - Small KML control is available in both mini-map and expanded map UI.
   - Imports local .kml files with Placemark/Polygon geometries.
   - Multiple polygons and MultiGeometry polygon sets are supported.
   - Loaded zones are rendered as translucent red polygons in 2D.
   - In GeoMap 3D, the same zones are extruded as terrain-aware vertical
     volumes using QGroundControl's native GeoMapFenceWall path.
   - KML relativeToGround altitude is used as the zone height when present.
   - Flat KML polygons can optionally use a configurable visual fallback
     height (default 120 m). This visual height is NOT an aircraft geofence.
   - Zone visibility can be toggled on/off without deleting the KML.
   - Focus button centers/zooms the map around the loaded KML.
   - KML overlay works in both the miniature 3D map and the expanded 3D map.

IMPORTANT SAFETY/SEMANTICS
--------------------------
The KML overlay is display-only in v0.12.2. It helps the operator see ANAC,
military, operational or other restricted areas, but it does NOT upload a
geofence to the Pixhawk and does NOT autonomously prevent the aircraft from
crossing a boundary.

The Direct A8 / Jetson payload route remains manual-only. There is no automatic
route detection or automatic failover.

INSTALL / UPDATE
----------------
Because v0.12.2 adds AtlasKmlWallGeometry C++ files, run qt-cmake once after extracting:

  unzip -o ~/Downloads/ATLAS_Payload_Mode_v0_12.zip -d ~/ATLAS/ATLAS-QGroundControl
  cd ~/ATLAS/ATLAS-QGroundControl
  python3 tools/atlas/verify_payload_bundle.py
  PATH="$HOME/Qt/Tools/CMake/bin:$PATH" ~/Qt/6.11.1/gcc_64/bin/qt-cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
  ~/Qt/Tools/CMake/bin/cmake --build build --parallel 2 && ./build/Release/ATLAS-QGroundControl

TEST ORDER
----------
A. Screen recording
   1. Open Media Settings (gear beside SCREEN REC).
   2. Start with 60 fps / Native / 16 Mbps.
   3. Record for 10-20 seconds without any A8/Jetson connected.
   4. Confirm the MP4 under ~/Videos/ATLAS and check effective FPS.
   5. Only then try 120/144 fps and watch effective FPS/system load.

B. KML
   1. Open map and press KML.
   2. Import a known .kml polygon.
   3. Confirm red polygon in 2D.
   4. Switch to 3D and confirm vertical red zone walls.
   5. Toggle Zones OFF/ON.
   6. Test Focus and fallback 3D height.

C. Direct A8 camera settings
   1. Select DIRECT A8.
   2. Ensure A8 is reachable at the configured endpoint.
   3. Open Media Settings -> SIYI A8 Camera -> Refresh camera.
   4. Verify MAIN/RECORD codec status is returned before changing anything.
   5. Change one setting at a time and verify the camera ACK/status refresh.

Final Qt/C++/QML compilation must still be performed with the project's Qt
6.11.1 toolchain on the user development machine.


v0.12.2 KML fidelity fix
------------------------
The KML renderer was rewritten after validating real Google Earth exports.

* KML altitude semantics are preserved:
  - absolute = metres AMSL from the coordinates in the file
  - relativeToGround = metres AGL above the terrain
  - clampToGround = terrain
* No fallback 120 m extrusion is injected.
* KML extrude is obeyed exactly.
* Shared Style / StyleMap colours, line widths, polygon fill/outline and
  point icons/labels are preserved.
* Polygon, LineString and Point geometries are all rendered.
* The KML Document/Folder/Placemark hierarchy is exposed as a layer tree with
  independent ON/OFF switches, similar to Google Earth.
* fill=0 extruded polygons render as wireframe vertical edges instead of
  translucent filled curtains. This is important for ARTEX-style airspace.
* Filled extruded polygons use terrain-to-top 3D wall geometry with correct
  AMSL or AGL top heights.

KML overlays remain visual-only and are not uploaded as Pixhawk geofences.

v0.12.2 BUILD FIX
-----------------
This revision fixes two Qt 6.11 AUTOMOC issues found on the real ATLAS build:

1. AtlasKmlWallGeometry exposes GeoScene* and SurfacePatchModel* as Q_PROPERTY
   types. Qt 6.11 requires those pointer meta-types to be complete while moc is
   compiling. The header now uses Q_MOC_INCLUDE("GeoScene.h") and
   Q_MOC_INCLUDE("SurfacePatchModel.h"), matching the native GeoMap classes.

2. A stale generated moc_AtlasKmlManager.cpp from v0.12 can still reference the
   removed zonesChanged/visualizationSettingsChanged/extrudeFlatZones/
   defaultHeightMeters API after the source header has been replaced. Before the
   first v0.12.2 build, remove only the ATLAS target AUTOGEN output and rerun
   qt-cmake. This does not delete the full build tree or downloaded dependencies:

   rm -rf build/ATLAS-QGroundControl_autogen build/CMakeFiles/ATLAS-QGroundControl_autogen.dir
   PATH="$HOME/Qt/Tools/CMake/bin:$PATH" ~/Qt/6.11.1/gcc_64/bin/qt-cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   ~/Qt/Tools/CMake/bin/cmake --build build --parallel 2

The KML hierarchy/style/absolute-vs-AGL altitude logic from v0.12.1 is otherwise
unchanged.
