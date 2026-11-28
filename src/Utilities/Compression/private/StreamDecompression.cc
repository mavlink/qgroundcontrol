#include "StreamDecompression.h"

#include <archive_entry.h>
#include <type_traits>

#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>

#include "EntryData.h"
#include "QGCFileHelper.h"

namespace LibArchive {

namespace {

/// Read the header of the single entry that a single-stream reader reads its input as
/// libarchive's raw format accepts any input that has bytes once it is decompressed, also input that no filter
/// recognizes, which it passes through unchanged. It has no entry for input that is empty once decompressed. The
/// reader reads that with libarchive's empty format, whose first header is the end of the archive. This covers
/// the stream a compressor writes for an empty file, and a file without any bytes.
/// A filter takes a stream only once its header is whole and sound. Input that starts with the signature of a stream
/// that no filter took is a damaged stream, not data, and fails.
/// @param hasData Set if the input is one entry, whose data can now be read
Result readStreamHeader(ArchiveReader& reader, bool& hasData)
{
    struct archive* const a = reader.handle();
    struct archive_entry* entry = nullptr;
    const int header = archive_read_next_header(a, &entry);
    if ((header != ARCHIVE_OK) && (header != ARCHIVE_EOF)) {
        return failure(Error::InvalidArchive, QStringLiteral("Failed to read header: ") + libarchiveError(a));
    }

    reader.captureFilter();
    if (archive_filter_count(a) <= 1) {
        if (const QString format = streamSignatureFormat(reader.streamStart()); !format.isEmpty()) {
            return failure(Error::InvalidArchive, damagedStreamHeader(format, "input"));
        }
        qCDebug(LibArchiveLog) << "Input is not compressed: it is taken as it is";
    }
    hasData = (header == ARCHIVE_OK);
    return {};
}

/// Write the output of input that holds no data: an empty file
Result writeEmptyFile(const QString& outputPath)
{
    QSaveFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly) || !outFile.commit()) {
        return writeFailure(
            outputPath, QStringLiteral("Failed to write output file: %1 %2").arg(outputPath, outFile.errorString()));
    }
    return {};
}

}  // namespace

Result decompressToFile(const QString& inputPath, const QString& outputPath, const ProgressCallback& progress,
                        qint64 maxBytes)
{
    ArchiveReader reader;
    Result read = reader.open(inputPath, Input::Stream);
    bool hasData = false;
    if (read) {
        read = readStreamHeader(reader, hasData);
    }
    if (read) {
        QGCFileHelper::ensureParentExists(outputPath);
        if (!hasData) {
            read = reader.verifyInput();
            if (read) {
                read = writeEmptyFile(outputPath);
            }
        } else {
            DataTransfer transfer = transferInto(QFileInfo(outputPath).absolutePath(), maxBytes, progress,
                                                 reader.dataSize(), "Decompression error:");
            read = copyEntryToFile(reader, outputPath, transfer, {}, true);
            if (read) {
                qCDebug(LibArchiveLog) << "Decompressed" << transfer.bytesWritten << "bytes to" << outputPath;
            }
        }
    }
    read.filterName = reader.filterName();
    return read;
}

Result decompressToSink(const QString& inputPath, qint64 maxBytes, const std::function<bool(QByteArrayView)>& sink)
{
    ArchiveReader reader;
    Result read = reader.open(inputPath, Input::Stream);
    bool hasData = false;
    if (read) {
        read = readStreamHeader(reader, hasData);
    }
    if (read && hasData) {
        DataTransfer transfer{.maxBytes = maxBytes};
        read = copyStreamData(reader, transfer, [&sink](const char* chunk, qint64 size) {
            return sink(QByteArrayView(chunk, static_cast<qsizetype>(size))) ? Result{} : failure(Error::Cancelled);
        });
    } else if (read) {
        read = reader.verifyInput();
    }
    read.filterName = reader.filterName();
    return read;
}

template <typename Source>
Result decompressToMemory(const Source& source, qint64 maxBytes, QByteArray& data)
{
    // The caller may pass the same array as source and output: this shared copy keeps the input
    const Source input = source;
    data.clear();
    ArchiveReader reader;
    Result read = reader.open(input, Input::Stream);
    bool hasData = false;
    if (read) {
        read = readStreamHeader(reader, hasData);
    }
    const bool compressed = read && (archive_filter_count(reader.handle()) > 1);
    DataTransfer transfer{.maxBytes = maxBytes};
    if constexpr (std::is_same_v<Source, QByteArray>) {
        // Data that is not compressed is returned as it is, shared and not copied, if the array owns its bytes. Those
        // of QByteArray::fromRawData(), which has no capacity, are the caller's, who may free them after the call:
        // they are copied
        if (read && hasData && !compressed && (input.capacity() != 0)) {
            read = transfer.fits(input.size());
            if (read) {
                data = input;
            }
            hasData = false;
        }
    } else if (read && hasData && !compressed && transfer.fits(reader.dataSize())) {
        // The size of a file that is not compressed is the size of its data
        data.reserve(static_cast<qsizetype>(reader.dataSize()));
    }
    if (read && hasData) {
        read = copyStreamData(reader, transfer, [&data](const char* block, qint64 size) {
            data.append(block, static_cast<qsizetype>(size));
            return Result{};
        });
    } else if (read) {
        read = reader.verifyInput();
    }
    if (!read) {
        data.clear();
    }
    qCDebug(LibArchiveLog) << "Decompressed to memory:" << data.size() << "bytes";
    read.filterName = reader.filterName();
    return read;
}

// Instantiations for the two sources: a file path, and bytes in memory
template Result decompressToMemory(const QString&, qint64, QByteArray&);
template Result decompressToMemory(const QByteArray&, qint64, QByteArray&);

}  // namespace LibArchive
