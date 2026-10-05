ATLAS Video Control v0.9
========================

This build adds native 2D/3D terrain switching to the ATLAS map, including the miniature map.

What changed
------------
- Mini-map has independent 2D / 3D selection.
- Expanded map has independent 2D / 3D selection.
- Preferences persist separately between launches.
- ATLAS Fly View now uses QGroundControl's native GeoMap engine so 3D is real terrain, not a fake perspective transform.
- 3D uses the normal selected map imagery draped over real elevation data.
- The existing target marker / UAV-target line continue to use the map overlay system.

Mini-map interaction
--------------------
- Left drag: pan.
- Mouse wheel: zoom.
- Short left click: expand map.
- 3D + right drag: orbit/rotate and adjust tilt.
- Floating 2D / 3D buttons select the miniature-map mode.
- Center / +/- controls remain available.

Expanded map interaction
------------------------
- 2D / 3D buttons select the full-map mode.
- N resets heading to north.
- CENTER UAV recenters on the active vehicle.
- <- VIDEO returns to video-primary Fly View.
- In 3D, the GeoMap engine's native pan/orbit/zoom gestures remain available.

3D data
-------
GeoMap is QGC's native Quick 3D terrain engine. It uses:
- standard QGC map imagery provider for the surface texture (Bing/Esri/Google/etc. depending configuration),
- real elevation data for terrain relief,
- the shared QGC tile cache.

The first 3D view can take a few moments to sharpen while imagery/elevation tiles arrive. Internet is required for uncached tiles.

This is NOT the older OSM-file Viewer3D path. No OSM file is required for the terrain mode added here.

Build
-----
No new CMake resources were added versus v0.8, so after copying the bundle into the repository a normal incremental build is enough:

  ~/Qt/Tools/CMake/bin/cmake --build build --parallel 2

Verification
------------
  python3 tools/atlas/verify_payload_bundle.py

Final runtime validation still requires the project's Qt 6.11.1 build on the development PC.
