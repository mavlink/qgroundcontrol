#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QTemporaryDir>

#include "../ArchiveFixtures.h"
#include "Compression.h"
#include "QGCCompressionFuzzTargets.h"

namespace {
struct Seed
{
    QString name;
    QByteArray bytes;
};

/// The seeds of the fuzzer: the compressed files and archives of the Compression tests and damaged copies of them,
/// the start of each format, streams of several parts, and archives whose entries an extraction must refuse or that
/// have the same name. A build without the TAR or 7z reader refuses those archives, which are seeds all the same
/// @param resourceDirectory Directory of the test files
/// @param resourcesRead Set to false if a test file cannot be read
QList<Seed> seeds(const QDir& resourceDirectory, bool& resourcesRead)
{
    QList<Seed> all{
        {"empty", {}},
        {"plain", "{\"name\": \"not compressed\"}\n"},
        {"gzip-header", QByteArray::fromHex("1f8b0800000000000003")},
        {"gzip-of-nothing-cut", QByteArray::fromHex("1f8b08000000000000030300")},
        // gzip members end with a checksum, which libarchive does not compare: "first\n" and "second\n", "stored\n"
        // in a block that is not compressed, "first\n" with another checksum, and "first\n" before other data
        {"gzip-two-members", QByteArray::fromHex("1f8b08000000000000034bcb2c2a2ee102002ab34ac706000000"
                                                 "1f8b08000000000000032b4e4dcecf4be102007ec00f0607000000")},
        {"gzip-stored", QByteArray::fromHex("1f8b0800000000000003010700f8ff73746f7265640ae29c53a507000000")},
        {"gzip-changed-checksum", QByteArray::fromHex("1f8b08000000000000034bcb2c2a2ee102007fb34ac706000000")},
        {"gzip-data-after", QByteArray::fromHex("1f8b08000000000000034bcb2c2a2ee102002ab34ac706000000") + "trailing"},
        {"xz-signature", QByteArray::fromHex("fd377a585a00") + QByteArray(32, 'x')},
        {"zstd-signature", QByteArray::fromHex("28b52ffd") + QByteArray(32, 'x')},
        // A skippable frame, which Zstandard and LZ4 share, alone and before a frame of each that holds "hi\n"
        {"skippable-only", QByteArray::fromHex("502a4d1804000000") + "skip"},
        {"skippable-zstd", QByteArray::fromHex("502a4d1804000000736b697028b52ffd200319000068690a")},
        {"skippable-lz4", QByteArray::fromHex("502a4d1804000000736b697004224d186040820300008068690a00000000")},
        // "hi\n" in the frame of LZ4 before version 1.4
        {"lz4-legacy", QByteArray::fromHex("02214c18040000003068690a")},
        {"zip-signature", QByteArray::fromHex("504b0304") + QByteArray(64, 'x')},
        {"7z-signature", QByteArray::fromHex("377abcaf271c") + QByteArray(64, 'x')},
        // "{}" in a legacy lzma stream, which has no magic number
        {"lzma", QByteArray::fromHex("5d00008000ffffffffffffffff003d9f5cfffffffff0000000")},
        {"zip", ArchiveFixtures::makeZip({{.name = "manifest.json", .data = "{}"}, {.name = "folder/"}})},
        {"zip-names", ArchiveFixtures::makeZip({{.name = "../escape.txt", .data = "escape"},
                                                {.name = "/tmp/qgc-fuzz-absolute.txt", .data = "absolute"},
                                                {.name = "folder/../../escape.txt", .data = "escape"}})},
        {"zip-links",
         ArchiveFixtures::makeZip({{.name = "inside", .data = ".", .unixMode = 0120777, .deflated = false},
                                   {.name = "climb", .data = "inside/..", .unixMode = 0120777, .deflated = false},
                                   {.name = "parent", .data = "..", .unixMode = 0120777, .deflated = false},
                                   {.name = "absolute", .data = "/", .unixMode = 0120777, .deflated = false},
                                   {.name = "parent/escape.txt", .data = "escape"}})},
        // The name is UTF-8, which the entry does not declare
        {"zip-name-beyond-ascii", ArchiveFixtures::makeZip({{.name = "data/\xC3\xBC"
                                                                     "bersicht.txt",
                                                             .data = "name",
                                                             .utf8Name = false}})},
        // An entry replaces the file that an earlier one of the same name wrote
        {"zip-same-name",
         ArchiveFixtures::makeZip({{.name = "a.txt", .data = "first"}, {.name = "a.txt", .data = "second"}})},
        {"zip-special",
         ArchiveFixtures::makeZip({{.name = "device", .unixMode = 020644}, {.name = "manifest.json", .data = "{}"}})},
        // The headers declare far more data than the entry holds
        {"zip-overstated",
         ArchiveFixtures::makeZip(
             {{.name = "manifest.json", .data = QByteArray(512, 'x'), .deflated = false, .declaredSize = 1 << 30}})},
        {"tar", ArchiveFixtures::makeTar({{.name = "manifest.json", .data = "{}"}, {.name = "folder/", .type = '5'}})},
        {"tar-names", ArchiveFixtures::makeTar({{.name = "../escape.txt", .data = "escape"},
                                                {.name = "/tmp/qgc-fuzz-absolute.txt", .data = "absolute"},
                                                {.name = "folder/../../escape.txt", .data = "escape"}})},
        {"tar-links", ArchiveFixtures::makeTar({{.name = "inside", .type = '2', .linkName = "."},
                                                {.name = "climb", .type = '2', .linkName = "inside/.."},
                                                {.name = "parent", .type = '2', .linkName = ".."},
                                                {.name = "absolute", .type = '2', .linkName = "/"},
                                                {.name = "hard.txt", .type = '1', .linkName = "../victim.txt"},
                                                {.name = "parent/escape.txt", .data = "escape"}})},
        // The name is UTF-8, which a TAR header does not declare
        {"tar-name-beyond-ascii", ArchiveFixtures::makeTar({{.name = "data/\xC3\xBC"
                                                                     "bersicht.txt",
                                                             .data = "name"}})},
        // An entry replaces the file that an earlier one of the same name wrote
        {"tar-same-name",
         ArchiveFixtures::makeTar({{.name = "a.txt", .data = "first"}, {.name = "a.txt", .data = "second"}})},
        // A sparse file, which is skipped, before a file
        {"tar-sparse",
         ArchiveFixtures::makeTar({{.name = "sparse.bin",
                                    .data = QByteArray(1024, 'x'),
                                    .type = 'S',
                                    .sparseData = {{.offset = 1024, .size = 512}, {.offset = 4096, .size = 512}},
                                    .sparseFileSize = 8192},
                                   {.name = "manifest.json", .data = "{}"}})},
        {"tar-special",
         ArchiveFixtures::makeTar({{.name = "fifo", .type = '6'}, {.name = "manifest.json", .data = "{}"}})},
        // The header declares far more data than follows it
        {"tar-overstated", ArchiveFixtures::tarHeader({.name = "manifest.json"}, 1 << 30) + QByteArray(512, 'x') +
                               QByteArray(1024, '\0')},
    };

    resourcesRead = true;
    for (const char* fileName : {"manifest.json.gz", "manifest.json.xz", "manifest.json.zst", "manifest.json.zip",
                                 "manifest.json.7z", "tree.zip", "tree.7z", "encrypted.zip", "encrypted.7z"}) {
        const QString name = QLatin1String(fileName);
        QFile file(resourceDirectory.filePath(name));
        if (!file.open(QIODevice::ReadOnly)) {
            resourcesRead = false;
            continue;
        }
        const QByteArray bytes = file.readAll();
        all.append({name, bytes});
        // Truncated, and with one byte changed in the header, in the middle and near the end
        all.append({name + QStringLiteral("-cut"), bytes.first(bytes.size() / 2)});
        for (const qsizetype offset : {qsizetype(4), bytes.size() / 2, bytes.size() - 6}) {
            QByteArray changed = bytes;
            changed[offset] = static_cast<char>(changed[offset] ^ 0x55);
            all.append({QStringLiteral("%1-changed-%2").arg(name).arg(offset), changed});
        }
    }
    return all;
}

/// Run both fuzz targets on @a bytes
void exercise(const QByteArray& bytes)
{
    const auto* const data = reinterpret_cast<const uint8_t*>(bytes.constData());
    const auto size = static_cast<size_t>(bytes.size());
    (void) CompressionFuzz::fuzzStream(data, size);
    (void) CompressionFuzz::fuzzFiles(data, size);
}

// The checks below look at what an operation gives, which the harness cannot know for any input.

bool writeFile(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && (file.write(bytes) == bytes.size());
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

/// Whether an extraction of @a archive writes its entry "data/übersicht.txt" under that name
/// libarchive converts entry names in a way that depends on the platform.
bool extractsNameBeyondAscii(const QByteArray& archive)
{
    const QTemporaryDir directory;
    const QString archivePath = directory.filePath(QStringLiteral("archive"));
    const QString outputPath = directory.filePath(QStringLiteral("output"));
    return writeFile(archivePath, archive) && Compression::extractArchive(archivePath, outputPath) &&
           QFileInfo::exists(outputPath + QStringLiteral("/data/\u00fcbersicht.txt"));
}

/// Whether an extraction of the seed "zip-same-name" or "tar-same-name" leaves the file of the later entry
/// How a file is moved onto another one depends on the platform.
bool replacesEarlierEntry(const QByteArray& archive)
{
    const QTemporaryDir directory;
    const QString archivePath = directory.filePath(QStringLiteral("archive"));
    const QDir output(directory.filePath(QStringLiteral("output")));
    return writeFile(archivePath, archive) && Compression::extractArchive(archivePath, output.path()) &&
           (readFile(output.filePath(QStringLiteral("a.txt"))) == "second") &&
           (output.entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot).size() == 1);
}

/// Whether decompressFile() refuses to write its output onto its input, also under another spelling of the path
/// How two paths are told to name one file depends on the platform.
bool refusesInputAsOutput(const QByteArray& stream)
{
    const QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("stream.gz"));
    return writeFile(path, stream) && (Compression::decompressFile(path, path).error == Compression::Error::IoError) &&
           (Compression::decompressFile(path, directory.filePath(QStringLiteral("./stream.gz"))).error ==
            Compression::Error::IoError) &&
           (readFile(path) == stream);
}

/// Whether decompressing @a stream gives @a content, or fails and gives nothing if @a content is null
bool decompressesTo(const QByteArray& stream, const char* content)
{
    QByteArray data;
    constexpr qint64 kMemoryLimit = qint64{1024} * 1024;
    const Compression::Outcome outcome = Compression::decompressData(stream, data, kMemoryLimit);
    return (content != nullptr) ? (outcome && (data == content)) : (!outcome && data.isEmpty());
}
}  // namespace

int main(int argc, char* argv[])
{
    // Makes the C locale UTF-8 where Qt can, as in the application: libarchive converts entry names to it
    const QCoreApplication application(argc, argv);

    const bool writeCorpus = (argc == 4) && (QByteArray(argv[2]) == "--write-corpus");
    if ((argc != 2) && !writeCorpus) {
        std::fputs("Usage: qgc_compression_fuzz_smoke RESOURCE_DIRECTORY [--write-corpus DIRECTORY]\n", stderr);
        return 1;
    }
    bool resourcesRead = false;
    const QList<Seed> corpus = seeds(QDir(QString::fromLocal8Bit(argv[1])), resourcesRead);
    if (!resourcesRead) {
        std::fprintf(stderr, "Test files are missing in %s\n", argv[1]);
        return 1;
    }

    if (writeCorpus) {
        const QDir directory(QString::fromLocal8Bit(argv[3]));
        if (!QDir().mkpath(directory.path())) {
            return 1;
        }
        for (const Seed& seed : corpus) {
            QFile file(directory.filePath(seed.name));
            if (!file.open(QIODevice::WriteOnly) || (file.write(seed.bytes) != seed.bytes.size())) {
                return 1;
            }
        }
        return 0;
    }

    for (const Seed& seed : corpus) {
        exercise(seed.bytes);
    }
    const auto seedBytes = [&corpus](const char* name) {
        const auto seed = std::ranges::find(corpus, QString::fromLatin1(name), &Seed::name);
        return (seed != corpus.cend()) ? seed->bytes : QByteArray();
    };
    // The archives of the tests declare the charset of the name, the seeds do not
    QList<const char*> namesBeyondAscii{"tree.zip", "zip-name-beyond-ascii"};
    QList<const char*> sameNames{"zip-same-name"};
#ifdef QGC_ENABLE_7ZIP
    namesBeyondAscii.append("tree.7z");
#endif
#ifdef QGC_ENABLE_TAR
    namesBeyondAscii.append("tar-name-beyond-ascii");
    sameNames.append("tar-same-name");
#endif
    for (const char* name : std::as_const(namesBeyondAscii)) {
        if (!extractsNameBeyondAscii(seedBytes(name))) {
            std::fprintf(stderr, "%s: the entry with a name beyond ASCII was not extracted under its name\n", name);
            return 1;
        }
    }
    for (const char* name : std::as_const(sameNames)) {
        if (!replacesEarlierEntry(seedBytes(name))) {
            std::fprintf(stderr, "%s: the later entry did not replace the earlier one\n", name);
            return 1;
        }
    }
    if (!refusesInputAsOutput(seedBytes("gzip-stored"))) {
        std::fputs("gzip-stored: a decompression onto the input file was not refused, or changed the file\n", stderr);
        return 1;
    }
    // What each gzip seed decompresses to: nothing for the one whose checksum does not match
    const std::pair<const char*, const char*> gzipSeeds[] = {{"gzip-two-members", "first\nsecond\n"},
                                                             {"gzip-stored", "stored\n"},
                                                             {"gzip-changed-checksum", nullptr},
                                                             {"gzip-data-after", "first\n"}};
    for (const auto& [name, content] : gzipSeeds) {
        if (!decompressesTo(seedBytes(name), content)) {
            std::fprintf(stderr, "%s: decompression did not give what the stream holds\n", name);
            return 1;
        }
    }
    uint32_t state = 0x51474343;
    for (const qsizetype length : std::array<qsizetype, 6>{1, 2, 7, 255, 8192, 65536}) {
        QByteArray bytes(length, Qt::Uninitialized);
        for (char& byte : bytes) {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            byte = static_cast<char>(state & 0xff);
        }
        exercise(bytes);
    }
    std::printf("Compression fuzz smoke: %lld seeds and 6 bounded byte streams passed\n",
                static_cast<long long>(corpus.size()));
    return 0;
}
