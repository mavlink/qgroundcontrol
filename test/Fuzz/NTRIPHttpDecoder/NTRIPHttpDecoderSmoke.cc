#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QList>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

namespace {
struct Seed
{
    const char* name;
    QByteArray wire;
};

QList<Seed> seeds()
{
    const QByteArray ok = "HTTP/1.1 200 OK\r\n";
    const QByteArray chunks = ok + "Transfer-Encoding: chunked\r\n\r\n";
    const QByteArray failure = "HTTP/1.1 503 Unavailable\r\n";
    return {
        {"empty", {}},
        {"identity", ok + "\r\nabc"},
        {"binary", ok + "\r\n" + QByteArray::fromHex("d300023ed09ef5c5")},
        {"length", ok + "Content-Length: 3\r\n\r\nabc"},
        {"duplicate-length", ok + "Content-Length: 3, 03\r\ncontent-length: 3\r\n\r\nabc"},
        {"zero-length", ok + "Content-Length: 0\r\n\r\n"},
        {"extra-body", ok + "Content-Length: 2\r\n\r\nabc"},
        {"truncated-length", ok + "Content-Length: 4\r\n\r\nabc"},
        {"chunked", chunks + "1\r\na\r\n2\r\nbc\r\n0\r\n\r\n"},
        {"chunk-extensions", chunks + "3;name=\"escaped\\\"value\";flag\r\nabc\r\n0\r\nX-End: yes\r\n\r\n"},
        {"truncated-chunk", chunks + "4\r\nabc"},
        {"bad-chunk-end", chunks + "3\r\nabc!\n"},
        {"bad-trailer", chunks + "0\r\nContent-Length: 0\r\n\r\n"},
        {"icy-binary", QByteArray("ICY 200 OK\r\n") + QByteArray::fromHex("d300023ed09ef5c5")},
        {"icy-ascii", "ICY 200 OK\r\nascii correction preamble"},
        {"icy-headers", "ICY 200 OK\r\nServer: test\r\n\r\npayload"},
        {"source-table", "SOURCETABLE 200 OK\r\nSTR;MP\r\n"},
        {"source-type", ok + "Content-Type: gnss/sourcetable\r\n\r\n"},
        {"informational", QByteArray("HTTP/1.1 100 Continue\r\n\r\n") + ok + "\r\nabc"},
        {"informational-limit", QByteArray("HTTP/1.1 100 Continue\r\n\r\n").repeated(5)},
        {"retry-seconds", failure + "Retry-After: 17\r\nContent-Length: 3\r\n\r\nwhy"},
        {"retry-date", failure + "Retry-After: Fri, 18 Sep 2026 12:00:17 GMT\r\n\r\nwhy"},
        {"retry-rfc850", failure + "Retry-After: Friday, 18-Sep-26 12:00:17 GMT\r\n\r\nwhy"},
        {"retry-asctime", failure + "Retry-After: Fri Sep 18 12:00:17 2026\r\n\r\nwhy"},
        {"retry-overflow", failure + "Retry-After: 18446744073709551616\r\n\r\n"},
        {"error-chunked", failure + "Transfer-Encoding: chunked\r\n\r\n3\r\nwhy\r\n0\r\n\r\n"},
        {"error-preview-limit", failure + "\r\n" + QByteArray(600, 'x')},
        {"conflicting-framing", ok + "Content-Length: 3\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n"},
        {"length-overflow", ok + "Content-Length: 18446744073709551616\r\n\r\n"},
        {"chunk-overflow", chunks + "10000000000000000\r\n"},
        {"unsupported-encoding", ok + "Content-Encoding: gzip\r\n\r\n"},
        {"invalid-status", "HTTP/1.1 2000 OK\r\n\r\n"},
        {"folded-header", ok + "X: value\r\n folded\r\n\r\n"},
        {"nul-header", ok + QByteArray("X: a\0b\r\n\r\n", 10)},
        {"line-limit", ok + "X: " + QByteArray(8187, 'x') + "\r\n\r\n"},
        {"oversized-line", ok + "X: " + QByteArray(8188, 'x') + "\r\n\r\n"},
        {"header-count-limit", ok + QByteArray("X: y\r\n").repeated(128) + "\r\n"},
        {"too-many-headers", ok + QByteArray("X: y\r\n").repeated(129) + "\r\n"},
        {"oversized-headers", ok + (QByteArray("X: ") + QByteArray(8187, 'x') + "\r\n").repeated(4) + "\r\n"},
    };
}

void exercise(const QByteArray& bytes)
{
    LLVMFuzzerTestOneInput(reinterpret_cast<const uint8_t*>(bytes.constData()), static_cast<size_t>(bytes.size()));
}
}  // namespace

int main(int argc, char* argv[])
{
    const auto corpus = seeds();
    if (argc == 3 && QByteArray(argv[1]) == "--write-corpus") {
        const QDir directory(QString::fromLocal8Bit(argv[2]));
        if (!QDir().mkpath(directory.path())) {
            return 1;
        }
        for (const auto& seed : corpus) {
            QFile file(directory.filePath(QString::fromLatin1(seed.name)));
            if (!file.open(QIODevice::WriteOnly) || file.write(seed.wire) != seed.wire.size()) {
                return 1;
            }
        }
        return 0;
    }
    if (argc != 1) {
        std::fputs("Usage: ntrip_http_decoder_smoke [--write-corpus DIRECTORY]\n", stderr);
        return 1;
    }
    for (const auto& seed : corpus) {
        exercise(seed.wire);
        exercise(seed.wire.first(seed.wire.size() / 2));
    }
    uint32_t state = 0x4e545249;
    for (const qsizetype length : std::array<qsizetype, 8>{0, 1, 2, 7, 255, 8192, 32768, 65536}) {
        QByteArray bytes(length, Qt::Uninitialized);
        for (char& byte : bytes) {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            byte = static_cast<char>(state & 0xff);
        }
        exercise(bytes);
    }
    std::printf("NTRIP HTTP fuzz smoke: %lld seeds, truncated prefixes, and 8 bounded byte streams passed\n",
                static_cast<long long>(corpus.size()));
    return 0;
}
