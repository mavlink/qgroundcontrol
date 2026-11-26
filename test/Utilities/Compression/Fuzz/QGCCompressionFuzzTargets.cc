#include "QGCCompressionFuzzTargets.h"

#include <cstdio>
#include <cstdlib>
#include <lzma.h>
#include <optional>
#include <zlib.h>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>

#include "BidAgreement.h"
#include "Compression.h"
#include "EntryNames.h"

namespace {
constexpr size_t MAX_INPUT_BYTES = 64 * 1024;
constexpr qint64 MAX_OUTPUT_BYTES = 1024 * 1024;

void require(bool condition)
{
    if (!condition) {
        std::abort();
    }
}

void discardMessage(QtMsgType, const QMessageLogContext&, const QString&) {}

/// Directory for the files an input is written to, and for the output
const QTemporaryDir& workDirectory()
{
    static const QTemporaryDir directory;
    require(directory.isValid());
    return directory;
}

void writeInput(const QString& path, const QByteArray& input)
{
    QFile file(path);
    require(file.open(QIODevice::WriteOnly) && (file.write(input) == input.size()));
}

/// The input as a file whose name is not recognized: its content identifies it as one kind at most
void kindOfFile(const QByteArray& input)
{
    const QString path = workDirectory().filePath(QStringLiteral("input"));
    writeInput(path, input);
    require(!Compression::isCompressedFile(path) || !Compression::isArchiveFile(path));
    require(QFile::remove(path));
}

/// The input, or nothing if it is larger than MAX_INPUT_BYTES. Messages are discarded from the first input on
std::optional<QByteArray> inputOf(const uint8_t* data, size_t size)
{
    static const QtMessageHandler previousHandler = qInstallMessageHandler(discardMessage);
    (void) previousHandler;
    if (size > MAX_INPUT_BYTES) {
        return std::nullopt;
    }
    return QByteArray(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
}

/// Whether @a data starts with the whole signature of a single stream, as Compression.h lists them
bool startsWithStreamSignature(const QByteArray& data)
{
    static const QByteArray signatures[] = {QByteArray::fromHex("1f8b"), QByteArray::fromHex("fd377a585a00"),
                                            QByteArray::fromHex("28b52ffd"), QByteArray::fromHex("04224d18"),
                                            QByteArray::fromHex("02214c18")};
    for (const QByteArray& signature : signatures) {
        if (data.startsWith(signature)) {
            return true;
        }
    }
    if (data.size() < 4) {
        return false;
    }
    const bool skippableFrame =
        ((static_cast<quint8>(data.at(0)) & 0xF0) == 0x50) && (data.mid(1, 3) == QByteArray::fromHex("2a4d18"));
    const bool bzip2 = data.startsWith("BZh") && (data.at(3) >= '1') && (data.at(3) <= '9');
    return skippableFrame || bzip2;
}

/// What zlib or liblzma, used directly, decode from a gzip or xz stream
struct ReferenceDecode
{
    bool sound = false;     ///< Every member or stream was decoded to its end
    bool tooLarge = false;  ///< The data is larger than MAX_OUTPUT_BYTES, and was not decoded to its end
    QByteArray data;
};

/// The members of a gzip stream as zlib inflates them, as long as what follows a member starts with the magic number
/// of one, or nothing if the input does not start with it
std::optional<ReferenceDecode> inflateGzip(const QByteArray& input)
{
    const QByteArrayView magic("\x1F\x8B");
    if (!input.startsWith(magic)) {
        return std::nullopt;
    }
    ReferenceDecode decoded;
    qsizetype offset = 0;
    while (QByteArrayView(input).sliced(offset).startsWith(magic)) {
        z_stream stream = {};
        require(inflateInit2(&stream, 16 + MAX_WBITS) == Z_OK);
        stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.constData() + offset));
        stream.avail_in = static_cast<uInt>(input.size() - offset);
        int result = Z_OK;
        while ((result == Z_OK) && !decoded.tooLarge) {
            char block[16 * 1024];
            stream.next_out = reinterpret_cast<Bytef*>(block);
            stream.avail_out = sizeof(block);
            result = inflate(&stream, Z_NO_FLUSH);
            decoded.data.append(block, static_cast<qsizetype>(sizeof(block) - stream.avail_out));
            decoded.tooLarge = decoded.data.size() > MAX_OUTPUT_BYTES;
        }
        offset = input.size() - stream.avail_in;
        (void) inflateEnd(&stream);
        if ((result != Z_STREAM_END) || decoded.tooLarge) {
            return decoded;
        }
    }
    decoded.sound = true;
    return decoded;
}

/// The streams of an xz file as liblzma decodes them, with the flags and the memory limit that libarchive gives it,
/// or nothing if the input does not start with the magic number of xz
std::optional<ReferenceDecode> decodeXz(const QByteArray& input)
{
    if (!input.startsWith(QByteArrayView("\xFD\x37\x7A\x58\x5A\x00", 6))) {
        return std::nullopt;
    }
    ReferenceDecode decoded;
    lzma_stream stream = LZMA_STREAM_INIT;
    require(lzma_stream_decoder(&stream, UINT64_MAX, LZMA_CONCATENATED) == LZMA_OK);
    stream.next_in = reinterpret_cast<const uint8_t*>(input.constData());
    stream.avail_in = static_cast<size_t>(input.size());
    lzma_ret result = LZMA_OK;
    while ((result == LZMA_OK) && !decoded.tooLarge) {
        char block[16 * 1024];
        stream.next_out = reinterpret_cast<uint8_t*>(block);
        stream.avail_out = sizeof(block);
        result = lzma_code(&stream, LZMA_FINISH);
        decoded.data.append(block, static_cast<qsizetype>(sizeof(block) - stream.avail_out));
        decoded.tooLarge = decoded.data.size() > MAX_OUTPUT_BYTES;
    }
    lzma_end(&stream);
    decoded.sound = (result == LZMA_STREAM_END) && !decoded.tooLarge;
    return decoded;
}

/// The module decodes a gzip or xz stream as zlib or liblzma do: it fails where they fail, and gives what they give,
/// or what it gives for that data when it is itself a stream that the module decodes
void compareWithReference(const QByteArray& input, const Compression::Outcome& outcome, const QByteArray& data)
{
    std::optional<ReferenceDecode> reference = inflateGzip(input);
    if (!reference) {
        reference = decodeXz(input);
    }
    if (!reference || reference->tooLarge) {
        return;
    }
    if (!reference->sound) {
        require(!outcome);
        return;
    }
    QByteArray inner;
    const Compression::Outcome innerOutcome = Compression::decompressData(reference->data, inner, MAX_OUTPUT_BYTES);
    // A stream inside the stream is decoded by the module's own rules, which the other checks cover
    if (innerOutcome.filterName.isEmpty() || (innerOutcome.filterName == QLatin1String("none"))) {
        require(static_cast<bool>(outcome) == static_cast<bool>(innerOutcome));
        require(data == inner);
    }
}

/// The input as a single stream in memory: a refused input gives no data, the output stays within the limit, an
/// input that is not compressed comes back unchanged, and no data that is given starts with the signature of a
/// stream: neither input that no decoder took nor what a decoder gave. A gzip or xz stream gives what zlib or liblzma
/// give for it. In a build with the optional decoders, the rules that refuse a stream without its decoder agree
/// with libarchive.
QByteArray decompressStream(const QByteArray& input, bool& decompressed)
{
    if (const QString disagreement = BidAgreement::disagreement(input); !disagreement.isEmpty()) {
        std::fprintf(stderr, "%s\n", qPrintable(disagreement));
        std::abort();
    }

    QByteArray data("stale");
    const Compression::Outcome outcome = Compression::decompressData(input, data, MAX_OUTPUT_BYTES);
    require(outcome || data.isEmpty());
    require(data.size() <= MAX_OUTPUT_BYTES);
    require(!outcome || (outcome.filterName != QLatin1String("none")) || (data == input));
    require(!outcome || !startsWithStreamSignature(data));
    compareWithReference(input, outcome, data);

    decompressed = static_cast<bool>(outcome);
    return data;
}

/// The input as a compressed file: decompressed to a file, it gives the same data as in memory, and a refused
/// input leaves no file. readFile() gives the same data too.
void decompressFile(const QByteArray& input, const QByteArray& stream, bool decompressed)
{
    const QString inputPath = workDirectory().filePath(QStringLiteral("input.gz"));
    const QString outputPath = workDirectory().filePath(QStringLiteral("output.bin"));
    writeInput(inputPath, input);

    const Compression::Outcome outcome = Compression::decompressFile(inputPath, outputPath, nullptr, MAX_OUTPUT_BYTES);
    require(static_cast<bool>(outcome) == decompressed);
    QFile output(outputPath);
    if (outcome) {
        require(output.open(QIODevice::ReadOnly) && (output.readAll() == stream));
        output.close();
        require(output.remove());
    } else {
        require(!output.exists());
    }

    QByteArray read("stale");
    const Compression::Outcome readOutcome = Compression::readFile(inputPath, read, MAX_OUTPUT_BYTES);
    require(static_cast<bool>(readOutcome) == decompressed);
    require(read == stream);
    require(QFile::remove(inputPath));
}

/// The input as an archive to extract, into a directory that is new or, for every other input, empty: whatever its
/// entries are named and link to, nothing appears beside the output directory and no link inside it, and every name
/// written is portable and differs from the others in more than case or normalisation. Whatever sizes the entries
/// declare, the files take no more than the limit. A failed extraction leaves nothing: no output directory that it
/// created, and nothing in one that was there
void extractArchive(const QByteArray& input)
{
    const QString archivePath = workDirectory().filePath(QStringLiteral("input.zip"));
    const QString outputPath = workDirectory().filePath(QStringLiteral("output"));
    writeInput(archivePath, input);
    const bool outputExists = (input.size() % 2) != 0;
    if (outputExists) {
        require(QDir().mkdir(outputPath));
    }

    const Compression::Outcome outcome =
        Compression::extractArchive(archivePath, outputPath, {.maxBytes = MAX_OUTPUT_BYTES});
    require(QFile::remove(archivePath));
    if (!outcome) {
        require(outputExists
                    ? (QFileInfo(outputPath).isDir() &&
                       QDir(outputPath).isEmpty(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot))
                    : !QFileInfo::exists(outputPath));
    }

    const QStringList beside =
        QDir(workDirectory().path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    require(beside.isEmpty() || (beside == QStringList{QStringLiteral("output")}));

    qint64 written = 0;
    QSet<QString> spellings;
    const QDir output(outputPath);
    QDirIterator entries(outputPath, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                         QDirIterator::Subdirectories);
    while (entries.hasNext()) {
        const QFileInfo entry = entries.nextFileInfo();
        require(!entry.isSymLink());
        const QString name = output.relativeFilePath(entry.filePath());
        require(LibArchive::nonPortableNameReason(name).isEmpty());
        require(!spellings.contains(LibArchive::nameCollisionKey(name)));
        spellings.insert(LibArchive::nameCollisionKey(name));
        if (entry.isFile()) {
            written += entry.size();
        }
    }
    require(written <= MAX_OUTPUT_BYTES);
    require(QDir(outputPath).removeRecursively());
}
}  // namespace

namespace CompressionFuzz {

int fuzzStream(const uint8_t* data, size_t size)
{
    if (const std::optional<QByteArray> input = inputOf(data, size)) {
        bool decompressed = false;
        (void) decompressStream(*input, decompressed);
    }
    return 0;
}

int fuzzFiles(const uint8_t* data, size_t size)
{
    const std::optional<QByteArray> input = inputOf(data, size);
    if (!input) {
        return 0;
    }

    kindOfFile(*input);

    bool decompressed = false;
    const QByteArray stream = decompressStream(*input, decompressed);
    decompressFile(*input, stream, decompressed);
    extractArchive(*input);
    return 0;
}

}  // namespace CompressionFuzz
