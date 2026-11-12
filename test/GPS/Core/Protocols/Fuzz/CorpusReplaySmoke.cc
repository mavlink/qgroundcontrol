#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QStringList>

// The smoke test of a fuzzer whose seeds are a committed corpus directory: every seed, each beginning of it and
// damaged copies of it go through the harness without libFuzzer. The directory is only read.

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

namespace {

/// Copies of each seed with one to four of their bits flipped.
constexpr int DAMAGED_COPIES = 64;

void replay(const QByteArray& input)
{
    LLVMFuzzerTestOneInput(reinterpret_cast<const uint8_t*>(input.constData()), static_cast<size_t>(input.size()));
}

}  // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::fprintf(stderr, "Usage: %s CORPUS_DIRECTORY\n", argv[0]);
        return 1;
    }
    const QDir corpus(QString::fromLocal8Bit(argv[1]));
    const QStringList names = corpus.entryList(QDir::Files, QDir::Name);
    if (names.isEmpty()) {
        std::fprintf(stderr, "No seeds in %s\n", argv[1]);
        return 1;
    }
    uint32_t state = 0x47505346;
    const auto random = [&state] {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    };
    for (const QString& name : names) {
        QFile file(corpus.filePath(name));
        if (!file.open(QIODevice::ReadOnly)) {
            std::fprintf(stderr, "Cannot read %s\n", qPrintable(file.fileName()));
            return 1;
        }
        const QByteArray seed = file.readAll();
        replay(seed);
        for (qsizetype size = 0; size < seed.size(); ++size) {
            replay(seed.first(size));
        }
        for (int copy = 0; copy < DAMAGED_COPIES && !seed.isEmpty(); ++copy) {
            QByteArray damaged = seed;
            for (int bit = 0; bit <= copy % 4; ++bit) {
                damaged[random() % damaged.size()] ^= static_cast<char>(1u << (random() % 8));
            }
            replay(damaged);
        }
    }
    std::printf("%lld seeds, their beginnings and damaged copies passed\n", static_cast<long long>(names.size()));
    return 0;
}
