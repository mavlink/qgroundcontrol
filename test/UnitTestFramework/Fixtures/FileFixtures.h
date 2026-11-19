#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

/// @file
/// @brief Files and file content that tests of several areas make

namespace TestFixtures {

/// Write a file, which is created or replaced
/// @return true if all of @a data was written
bool writeFile(const QString& path, const QByteArray& data);

/// @return Content of a file or Qt resource, empty if it cannot be read
QByteArray readFile(const QString& path);

/// @return CRC-32 of @a data as gzip and ZIP store it
quint32 crc32(const QByteArray& data);

/// @return One member of a gzip file, holding @a payload. A gzip file is one or more of them.
QByteArray gzipMember(const QByteArray& payload);

}  // namespace TestFixtures
