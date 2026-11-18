#include "FileFixtures.h"

#include <span>

#include <QtCore/QFile>

#include "CRC32.h"

namespace TestFixtures {

bool writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && (file.write(data) == data.size());
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

quint32 crc32(const QByteArray& data)
{
    const std::span<const uint8_t> bytes(reinterpret_cast<const uint8_t*>(data.constData()),
                                         static_cast<size_t>(data.size()));
    return ~QGC::crc32Update(bytes, 0xFFFFFFFFU);
}

QByteArray gzipMember(const QByteArray& payload)
{
    // qCompress() output: 4-byte length, 2-byte zlib header, deflate stream, 4-byte Adler-32
    const QByteArray zlibStream = qCompress(payload);
    QByteArray member = QByteArray::fromHex("1f8b08000000000000ff");
    member.append(zlibStream.mid(6, zlibStream.size() - 10));
    for (const quint32 field : {crc32(payload), static_cast<quint32>(payload.size())}) {
        for (int shift = 0; shift < 32; shift += 8) {
            member.append(static_cast<char>((field >> shift) & 0xFFU));
        }
    }
    return member;
}

}  // namespace TestFixtures
