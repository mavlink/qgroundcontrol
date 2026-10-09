#pragma once

#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QString>

// The protocol suites' data under test/GPS/Core/Protocols, which QGCGPSProtocolTestSupport passes in
// GPS_PROTOCOL_TEST_DIR.
namespace GPSTest {

/// Recorded and synthetic receiver captures with independently decoded expectations.
inline constexpr char FIXTURE_DIR[] = GPS_PROTOCOL_TEST_DIR "/fixtures";

/// Synthetic receiver traffic: the golden decode inputs and the fuzzer's seed corpus.
inline constexpr char CORPUS_DIR[] = GPS_PROTOCOL_TEST_DIR "/corpus";

inline constexpr char GOLDEN_DIR[] = GPS_PROTOCOL_TEST_DIR "/golden";

/// The bytes of the file at @a path, or nothing when it cannot be read.
inline std::optional<QByteArray> readFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    return file.readAll();
}

/// The bytes of @a name in @a directory, one of the directories above; empty when it cannot be read.
inline QByteArray dataFile(const char* directory, const QString& name)
{
    return readFile(QString::fromUtf8(directory) + u'/' + name).value_or(QByteArray());
}

}  // namespace GPSTest
