# What's New

This page highlights user-facing changes since the last stable release (V5.1).

## GeoMap Engine (Tech Preview)

A new experimental map engine that renders a single, seamless 2D/3D map: your selected map imagery draped over real terrain elevation, with Google Earth-style camera controls.
When enabled it replaces the Fly View map, displaying vehicles, the planned mission, the flown flight path, launch and ground station locations in true 3D.
Map click actions are available in 2D mode only (3D is view-only for now), and the goal is for the engine to eventually replace all map usage in QGroundControl.
It is disabled by default — see [GeoMap Engine (Tech Preview)](../fly_view/geoview.md) for how to enable it and full details.

## Fly View

### Click to ROI Altitude

The **ROI at location** map click action now supports setting the ROI altitude.
An altitude slider (meters above home, defaulting to 0 = ground level) is shown before you confirm the ROI, so the vehicle can point at an elevated target instead of only ground points.

### Vehicle Messages Read/Unread

Vehicle messages now have a read/unread state. Unread messages are shown in bold in the **Flight Status** indicator's message list.
Press **Mark as Read** to mark them read while keeping the messages, instead of having to delete them. **Clear** still deletes them.
See [Vehicle Messages](../fly_view/fly_view_toolbar.md#vehicle-messages) for details.

### Flight Status Severity Label

The red count of critical messages on the **Flight Status** indicator is replaced by a label showing the most severe issue: **Error**, **Warning** or **Info**.
It combines unread vehicle messages, health and arming checks, and an incomplete pre-flight checklist, so it stays shown until those issues are resolved, not just until messages are marked read.
See [Flight Status Badges](../fly_view/fly_view_toolbar.md#flight-status-badges) for details.

## Telemetry Logs

### Log Files Named for Flight Start

Telemetry logs (`.tlog`) are now named for the time recording started, instead of the time the log was saved at disconnect.
See [Telemetry Logs from Vehicle](../settings_view/general.md#telemetry_logs) for details.

## Offline Maps

### Import Tile Sets from ZIP Archives

**Import** on the Offline Maps page now also takes a ZIP archive (`.zip`) that holds a tile set file, where it only took tile set files before.
The archive must hold one tile set file, and may unpack to at most 20 times its own size (64 MiB at least).
Entries whose names not every operating system can write as files are skipped.
See [Offline Maps](../settings_view/maps.md#offline-maps) for details.

## Plan, Settings and MAVLink Action Files

### Compressed Files No Longer Read

Plan files, settings files and the MAVLink actions file are read as plain JSON only.
A file compressed with gzip or xz is refused: a compressed plan or actions file shows an error, and a compressed settings file is skipped with a warning in the log.

## Firmware

### ArduPilot Firmware List

The **Firmware** page downloads the ArduPilot firmware list only when ArduPilot is shown as the firmware to flash, and reads it without freezing the page.

## Custom Builds

### Custom Settings Pages

Custom builds can now add their own pages to Application Settings, and replace, reposition, or remove the stock pages, using the same JSON-driven page generation as regular QGC — no hand-written settings QML required.
See [Generated Settings Pages](../../qgc-dev-guide/views/settings_generation.md#custom-build-settings-pages) in the developer guide for details.

### Compression and File API

A custom build that uses the decompression module, MAVLink FTP, the JSON readers or the events metadata meets these changes:

- `namespace Compression` (`Compression.h`) replaces `QGCCompression`.
  It decompresses a stream with `decompressFileToFile()`, `decompressFileToMemory()`, `decompressFileToSink()` and `decompressDataToMemory()`, and extracts an archive with `extractArchive()`, which takes an `ExtractOptions` (size limit, entry filter, progress).
  Each returns a `Compression::Outcome` in place of a `bool` or the data and `lastError()`, and every read into memory takes a size limit.
  `CompressionJob` runs an extraction or a decompression on a worker thread.
- `QGCArchiveWatcher`, `QGCArchiveModel`, `QGCArchiveFile`, `QGCDecompressDevice` and `QGCArchiveDeviceBase` are removed, and so are compression, archive listing and extraction by file name.
- `JsonParsing::readJsonFile()`, `readJsonText()` and `parseJson()` read JSON as it is and refuse compressed content.
  `JsonParsing::isJsonFile(path)` decompresses, takes a size limit, and is meant for files that a vehicle sends.
  `isJsonFile(bytes)` is removed: use `parseJson()`.
- `FTPManager::download()`, `upload()`, `listDirectory()` and `deleteFile()` take an owner and a callback, and return an `FTPRequest` that reports its progress and ends with `cancel()`.
  The signals `downloadComplete`, `uploadComplete`, `listDirectoryComplete`, `deleteComplete` and `commandProgress`, and the `cancel...()` functions of `FTPManager`, are removed.
- `Vehicle::setEventsMetadata()` and `MAVLinkEventManager::setMetadata()` take the parsed metadata from `EventHandler::loadMetadata()` in place of a file name.
- `QGCFileDownload` names a file after the last component of its URL, or `DownloadedFile` if that name is not one that every system writes as it is.
  With `autoDecompress`, it decompresses on a worker thread, up to `setMaxDecompressedBytes()` (1 GiB by default), and a failed decompression fails the download and leaves no file.
- The build options `QGC_ENABLE_ZSTD`, `QGC_ENABLE_BZIP2`, `QGC_ENABLE_LZ4`, `QGC_ENABLE_LZMA` (legacy `.lzma` streams), `QGC_ENABLE_7ZIP` and `QGC_ENABLE_TAR` are off by default: a default build reads gzip, xz and ZIP only.
- `QGC_USE_SYSTEM_LIBS` now uses an installed zlib, liblzma or libarchive that is at least as new as QGroundControl needs, where it built the bundled copy without a word, and it no longer stops the configure on a host with expat installed.
