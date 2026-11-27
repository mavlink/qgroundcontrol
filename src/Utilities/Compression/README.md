# Compression

Decompresses a single compressed stream, from a file or from memory, into memory or into a file, and extracts an
archive into a directory. Every operation reads its input with [libarchive](https://libarchive.org). The module
only decompresses: QGC does not create compressed files.

The application uses it for:

- the files that a vehicle sends or points to, which may be compressed and are decompressed where they are read.
  Its component metadata, their translations and its events metadata are downloaded and cached as they are sent,
  and read so (`JsonParsing::isJsonFile()` reads the JSON among them). A camera definition is downloaded as it is
  sent, and cached decompressed once it has loaded;
- the ArduPilot firmware list, 92 MiB decompressed, which `decompressFileToSink()` gives to its reader a chunk at a
  time, so that it is never held whole;
- offline map tile sets imported from an archive (`QGCMapEngineManager`).

Everything else is read as it is. Settings, plans, parameter metadata, QGC's own JSON files and terrain tiles never
arrive compressed, and `JsonParsing::readJsonFile()` refuses a file that is.

QGC decompresses a file where it reads it, and keeps no decompressed copy of a download but the camera definitions
it caches. `QGCFileDownload` can also decompress a download into a file (`autoDecompress`, also through
`QGCCachedFileDownload`) for a caller that needs the decompressed file on disk. It is off by default, and nothing in
QGC turns it on.

## What it reads

| Input | Formats |
| ----- | ------- |
| Single stream | gzip and xz. Legacy lzma, Zstandard, bzip2 and LZ4 if the build enables them |
| Archive | ZIP. 7z and TAR, the last also compressed as a whole (`.tar.gz`, `.tar.xz`, ...), if the build enables them |

Everything PX4, ArduPilot and MAVLink give QGC is a gzip or xz stream, and offline map tile sets are imported from
ZIP archives; the other formats are kept for later use.

## Using it

`Compression.h` is the one header to include, and with `CompressionTypes.h` and `CompressionJob.h` the contract of
the module: what each function reads and writes, its limits, what it leaves after a failure, and on which thread it
runs. Read them before you use it; this file does not repeat them.

The functions block, and QGC calls them by one rule. A file that a vehicle sends or points to, and the ArduPilot
firmware list, are read on a worker thread (`QGC::runOnWorker()`): their size is not QGC's to choose. Work that can
take minutes, or that the user follows and can cancel, such as the import of a tile set, runs as a `CompressionJob`.
Nothing in QGC decompresses or extracts on the GUI thread.

## Layout

| File | Content |
| ---- | ------- |
| `Compression.h`, `Compression.cc` | The functions and the messages |
| `CompressionTypes.h` | `Error`, `Outcome`, the default file limit and the callback types |
| `CompressionJob.h`, `CompressionJob.cc` | The worker-thread jobs |
| `CompressionLibraries.cmake` | libarchive and its decoders |
| `private/` | The only code that calls libarchive |

In `private/`, `Formats` holds the format table: how each format is named and how its content starts, which
formats the build reads (`readFormats()`, the one place that decides it), the filters and formats a reader
registers, and the streams the build cannot decode. `ArchiveReader` owns the libarchive reader and the file it
reads; each operation opens one of its own. `GzipCheck` checks the checksums of gzip input with zlib, which
libarchive does not (libarchive/libarchive#3728). `EntryData` copies the data of an entry to memory or to a file,
within the limits, and `EntryNames` tells the entry names that every system writes alike. `StreamDecompression` and
`ArchiveExtraction` are the two kinds of operation. Each step returns a `Result` (`ArchiveReader.h`), and
`Operation` builds the `Outcome` from the one the operation ends with. Code outside the module cannot include
these headers, except its tests.

## Libraries and build options

`CompressionLibraries.cmake` fetches libarchive, zlib, liblzma and the decoders of the options below
with CPM and builds them as static libraries, without tools, encoders or features QGC does not use.
Renovate proposes updates of the version pins, except of bzip2, which is pinned to a commit. The application carries
the licence of each bundled library beside GStreamer's (`qgc_install_license()` in `cmake/Helpers.cmake`): in
`share/licenses/<library>` of the install tree, which the AppImage, the `.deb`, `.rpm` and Windows packages hold, in
`Contents/Resources/licenses` of the macOS bundle, `licenses` of the iOS bundle and `assets/licenses` of the APK.

With `QGC_USE_SYSTEM_LIBS` it uses each installed library that is at least as new as the module needs, and
downloads the others; `QGC_SYSTEM_LIBS_ONLY` fails the configure instead. The configure prints which copy of each
library the build uses. The minimums are the oldest releases with what the module and libarchive call: zlib 1.2.1,
liblzma 5.0.0 (5.6.0 and 5.6.1, the releases with the backdoor, fail the configure), libarchive 3.7.5 built with
zlib and liblzma, Zstandard 1.0.0, LZ4 1.7.0 and bzip2 1.0.0. Zstandard and LZ4 are found by the CMake packages they
install, which not every distribution ships. The distribution keeps an installed copy free of known vulnerabilities,
often without a new version number, and its Zstandard may also read the legacy formats the bundled one leaves out.
`BidAgreement` compares the module's rules with those of the libarchive in use, so it can fail with an older one.

On Linux and Android every binary that links the module keeps the symbols of these static libraries to itself
(`--exclude-libs`): Qt and the other libraries in the process use the system's zlib, never a mix of both, and the
Android app library does not export them. A check after the link of the application fails if it exports them
again (`cmake/BundledDecoderExports.cmake`). Windows exports only what is marked for export, and macOS and iOS bind
each library's symbols to the library it was linked with, so they need neither.

| Option | Default | Adds |
| ------ | ------- | ---- |
| `QGC_ENABLE_ZSTD` | `OFF` | Zstandard streams |
| `QGC_ENABLE_BZIP2` | `OFF` | bzip2 streams |
| `QGC_ENABLE_LZ4` | `OFF` | LZ4 streams |
| `QGC_ENABLE_LZMA` | `OFF` | Legacy `.lzma` streams. xz streams are always read |
| `QGC_ENABLE_7ZIP` | `OFF` | 7z archives |
| `QGC_ENABLE_TAR` | `OFF` | TAR archives, plain or compressed as a whole with a decoder the build has |

libarchive has no build switch for a format: the options decide which formats and filters the module registers,
and libarchive links only those. The `Linux-sanitizers` preset turns all six options on, so CI tests those formats
too; the other builds test how they are refused.

## Logging

The module logs in `Utilities.Compression` (the functions), `Utilities.LibArchive` (reading,
writing and every skipped entry) and `Utilities.CompressionJob`.
`--logging:Utilities.Compression,Utilities.LibArchive,Utilities.CompressionJob` enables all three.

## Tests

`test/Utilities/Compression` holds `QGCCompressionTest` (file kinds, single streams, what all
functions have in common), `QGCArchiveExtractionTest` and `QGCCompressionJobTest`. `QGCFileDownloadTest` and
`QGCCachedFileDownloadTest` (`test/Utilities/FileSystem`) test `autoDecompress`. `ArchiveFixtures`
builds ZIP and TAR archives in memory for them. The extraction tests run on ZIP in every build; those of
what only TAR or 7z has run in a build that enables it. The three suites are also built as
[portable tests](../../../test/README.md#portable-utility-tests), which Windows and the macOS Release
build run.

`Fuzz/` holds two libFuzzer targets, one in memory and one on files, and `BidAgreement`, which compares the
rules by which a build without a decoder refuses its streams with libarchive's own. The test build runs both
targets on fixed inputs as `QGCCompressionFuzzSmoke`.
[test/README.md](../../../test/README.md#compression-fuzzing) describes the harness and how to fuzz.
