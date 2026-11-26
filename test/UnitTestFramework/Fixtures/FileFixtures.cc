#include "FileFixtures.h"

#include <filesystem>
#include <span>
#include <system_error>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include "CRC32.h"

namespace TestFixtures {

bool writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && (file.write(data) == data.size());
}

bool writeExecutable(const QString& path, const QByteArray& script)
{
    return writeFile(path, script) &&
           QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

bool createSymbolicLink(const QString& targetPath, const QString& linkPath)
{
    const QFileInfo target(targetPath);
    const std::filesystem::path link = QFileInfo(linkPath).filesystemAbsoluteFilePath();
    std::error_code error;
    if (target.isDir()) {
        std::filesystem::create_directory_symlink(target.filesystemAbsoluteFilePath(), link, error);
    } else {
        std::filesystem::create_symlink(target.filesystemAbsoluteFilePath(), link, error);
    }
    return !error;
}

quint32 crc32(const QByteArray& data)
{
    const std::span<const uint8_t> bytes(reinterpret_cast<const uint8_t*>(data.constData()),
                                         static_cast<size_t>(data.size()));
    return ~QGC::crc32Update(bytes, 0xFFFFFFFFU);
}

QByteArray deflate(const QByteArray& data)
{
    // qCompress() output: 4-byte length, 2-byte zlib header, deflate stream, 4-byte Adler-32
    const QByteArray zlibStream = qCompress(data);
    return zlibStream.mid(6, zlibStream.size() - 10);
}

QByteArray gzipMember(const QByteArray& payload)
{
    QByteArray member = QByteArray::fromHex("1f8b08000000000000ff");
    member.append(deflate(payload));
    for (const quint32 field : {crc32(payload), static_cast<quint32>(payload.size())}) {
        for (int shift = 0; shift < 32; shift += 8) {
            member.append(static_cast<char>((field >> shift) & 0xFFU));
        }
    }
    return member;
}

}  // namespace TestFixtures
