#include "QGCCompressionFuzzTargets.h"

#include <cstdio>
#include <cstdlib>
#include <optional>

#include <QtCore/QByteArray>
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

/// The input as a single stream in memory: a refused input gives no data, the output stays within the limit, and
/// an input that is not compressed comes back unchanged. In a build with the optional decoders, the rules that refuse
/// a stream without its decoder agree with libarchive.
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
