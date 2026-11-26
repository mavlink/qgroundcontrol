# Compression

Decompresses a single compressed stream, from a file or from memory, into memory or into a file, and extracts an
archive into a directory. Every operation reads its input with [libarchive](https://libarchive.org). The module
only decompresses: QGC does not create compressed files.

The application uses it for:

- files and data that may be compressed, which are decompressed where they are read: parameter
  metadata, terrain tiles, the images that geotagging reads, and any JSON that
  `JsonParsing::isJsonFile()` reads. The component metadata of a vehicle, its translations, its events
  metadata and camera definitions are downloaded as they are sent, also into the caches, and read so;
- offline map tile sets imported from an archive (`QGCMapEngineManager`).

QGC decompresses a file where it reads it, and keeps a download as it was sent. `QGCFileDownload` can also
decompress a download into a file (`autoDecompress`, also through `QGCCachedFileDownload`) for a caller that needs
the decompressed file on disk. It is off by default, and nothing in QGC turns it on.

## What it reads

| Input | Formats |
| ----- | ------- |
| Single stream | gzip and xz. Legacy lzma, Zstandard, bzip2 and LZ4 if the build enables them |
| Archive | ZIP. 7z and TAR, the last also compressed as a whole (`.tar.gz`, `.tar.xz`, ...), if the build enables them |

Everything PX4, ArduPilot and MAVLink give QGC is a gzip or xz stream, and offline map tile sets are imported from
ZIP archives; the other formats are kept for later use. An encrypted archive is refused. No external program is
ever run to decompress.

## Using it

`Compression.h` is the one header to include. Its functions block until they are done:

- `isArchiveFile()` and `isCompressedFile()` tell whether a file is an archive or a single compressed stream,
  `strippedPath()` where such a stream decompresses to, and `archiveNamePatterns()` which archives the build
  extracts;
- `readFile()` and `decompressData()` give the decompressed content in memory, and `decompressFile()` writes it
  into a file;
- `extractArchive()` extracts an archive into a new or empty directory. Its optional `ExtractOptions` set a
  size limit, an entry filter and a progress callback.

Each returns an `Outcome` (`CompressionTypes.h`), which converts to `true` on success and otherwise
holds an error code, a translated message and what libarchive or the system reported:

```cpp
QByteArray json;
if (const Compression::Outcome read = Compression::readFile(path, json); !read) {
    qCWarning(MyLog) << read.description();
    return;
}
```

`CompressionJob` runs `extractArchive()` or `decompressFile()` on a worker thread. Use it for anything large, and
for anything started on the GUI thread. `CompressionJob::startExtraction()` and
`CompressionJob::startDecompressionConsumingInput()` start a job, which belongs to the context it is given,
reports its progress with `progressChanged()`, calls the callback once with the outcome and then deletes itself.
`cancel()` on the job, deleting it or deleting the context all end it the same way: the callback is not called.

```cpp
_job = CompressionJob::startExtraction(this, archivePath, _outputDirectory, 0, nullptr,
                                       [this](const Compression::Outcome& outcome) { ... });
connect(_job, &CompressionJob::progressChanged, this, &MyClass::_showProgress);
// Later, if the result is no longer wanted. _job is a QPointer: the job deletes itself
if (_job) {
    _job->cancel();
}
```

An extraction job that ends without calling back leaves what its worker had extracted once it had completed. A
failed or cancelled extraction leaves nothing, as always. Extract into a directory of your own, and
remove it when the job ends without calling back.

`CompressionJob::startDecompressionConsumingInput()` runs `decompressFile()` and consumes its input, as
`QGCFileDownload` needs for `autoDecompress`. The job takes the input by renaming it within its directory,
in one attempt under a free name of its own, so that a file written to the input path meanwhile, such as the next
download of that name, is not the job's. It removes the input once the worker has ended, however the job ends. An
input that cannot be renamed is read where it is and removed only when the job calls back. A job that ends without
calling back leaves no output. A failure message names the input path the caller gave. The output path is
required: a job without one, or with one that names the input file, fails with `Error::IoError`.

## What to know before you use it

`Compression.h` documents each of these in full.

- **The content decides, not the name.** libarchive detects the format. Only `isArchiveFile()`,
  `isCompressedFile()` and `strippedPath()` look at the name first. They know every format, also one that the
  build does not read.
- **A format the build does not read fails with `Error::UnsupportedFormat`**: a single stream of legacy lzma,
  Zstandard, bzip2 or LZ4, also one inside a gzip or xz stream (`data.lzma.gz`), and an archive whose name or
  signature shows that it is 7z or TAR. It is never taken for input that is not compressed.
  `archiveNamePatterns()` lists the archives the build extracts.
- **Input that is not compressed is not an error** for the single-stream functions. It is returned or
  copied as it is, and `Outcome::filterName` is `"none"`. A download whose content encoding the
  HTTP client already removed is such a file.
- **Damaged input gives nothing.** A truncated or corrupt stream fails, and no partial data or file is
  left. libarchive does not compare the checksum that ends a gzip stream, so the module does, also
  for a TAR archive that is compressed with gzip.
- **Size limits.** Decompressed data is limited to 256 MiB in memory and 1 GiB in a file unless the caller passes
  another limit.
  The readers of the metadata of a vehicle, its translations, its events metadata and its camera
  definitions, which parse them whole, pass `kMaxSmallFileBytes` (64 MiB). A Copernicus terrain
  tile passes `TerrainTileCopernicus::kMaxResponseBytes` (1 MiB). `QGCFileDownload::setMaxDecompressedBytes()`
  limits what `autoDecompress` writes. An extraction has no limit by default. An extraction and
  `decompressFile()` also stop before they fill the disk, with `Error::DiskSpaceExceeded`.
- **An extraction needs a new or empty directory.** A directory with anything in it, or a file in its
  place, is refused with `Error::IoError` before the archive is read, and left as it was.
- **An extraction writes files and directories only, inside the output directory.** Links, special
  files, sparse files and entries whose path leads outside are skipped with a warning. Entry names are
  read as UTF-8 unless the archive declares another charset.
- **A failed extraction leaves nothing.** An output directory that it created is removed, and one that
  was there is emptied.
- **Extract into a directory that only QGC writes to.** The directory is checked only at the start,
  not against another process that changes it during the extraction.
- **The input** can be a Qt resource (`:/...`), or a file that cannot seek, such as a pipe. Only a
  7z archive needs a file that can seek. `decompressFile()` refuses an output path that names its input, also
  through a link or in another spelling, with `Error::IoError`, and writes its output atomically.
- **A job runs one operation.** Deleting it cancels the operation and waits for the worker, which
  stops at its next block of data or entry. An entry filter given to a job is called on the worker
  thread. An exception it throws ends the job with `Error::InternalError`.

## Layout

| File | Content |
| ---- | ------- |
| `Compression.h`, `Compression.cc` | The functions and the messages |
| `CompressionTypes.h` | `Error`, `Outcome`, the default limits and the callback types |
| `CompressionJob.h`, `CompressionJob.cc` | The worker-thread jobs |
| `CompressionLibraries.cmake` | libarchive and its decoders |
| `private/` | The only code that calls libarchive |

In `private/`, `Formats` holds the format table: how each format is named and how its content starts, which
formats the build reads, the filters a reader registers, and the streams the build cannot decode. `ArchiveReader`
owns the libarchive reader and the file it reads. `GzipCheck` checks the checksums of gzip input with zlib, which
libarchive does not (libarchive/libarchive#3728). `EntryData` copies
the data of an entry to memory or to a file, within the limits. `StreamDecompression` and
`ArchiveExtraction` are the two kinds of operation. Each step returns a `Result` (`ArchiveReader.h`), and
`Operation` builds the `Outcome` from the one the operation ends with. Code outside the module cannot include
these headers.

## Libraries and build options

`CompressionLibraries.cmake` fetches libarchive, zlib, liblzma and the decoders of the options below
with CPM and builds them as static libraries, without tools, encoders or features QGC does not use.
With `QGC_USE_SYSTEM_LIBS` it uses installed copies where it finds them. Renovate proposes updates of
the version pins, except of bzip2, which is pinned to a commit.

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

A failure is logged at debug level only: the `Outcome` holds all of it, and the caller logs or shows its
`description()` where that is worth it. The module logs a warning only for what an operation that succeeds
cannot report: an entry that an extraction skips, one that libarchive reads with a warning, and the input of a
decompression job that it could not remove.

## Tests

`test/Utilities/Compression` holds `QGCCompressionTest` (file kinds, single streams, what all
functions have in common), `QGCArchiveExtractionTest` and `QGCCompressionJobTest`. `QGCFileDownloadTest` and
`QGCCachedFileDownloadTest` (`test/Utilities/FileSystem`) test `autoDecompress`. `ArchiveFixtures`
builds ZIP and TAR archives in memory for them. The extraction tests run on ZIP in every build; those of
what only TAR or 7z has run in a build that enables it. The three suites are also built as
[portable tests](../../../test/README.md#portable-utility-tests), which Windows and the macOS Release
build run.

`Fuzz/` holds a libFuzzer harness. The test build runs it on fixed inputs as
`QGCCompressionFuzzSmoke`.
[test/README.md](../../../test/README.md#compression-fuzzing) describes the harness and how to fuzz.
