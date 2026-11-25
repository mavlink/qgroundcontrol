#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

/// @file
/// @brief Files and file content that tests of several areas make

namespace TestFixtures {

/// Write a file, which is created or replaced
/// @return true if all of @a data was written
bool writeFile(const QString& path, const QByteArray& data);

/// Write the file @a name in the directory @a dirPath, as writeFile() does
/// @return Path of the file, empty if it could not be written
QString writeFileIn(const QString& dirPath, const QString& name, const QByteArray& data);

/// Write a file that its owner may run: a script that stands in for a program
/// @return true if all of @a script was written and the file made executable
bool writeExecutable(const QString& path, const QByteArray& script);

/// Copy the test resource ":/unittest/<name>" to the directory @a dirPath, replacing a file of that name there.
/// The copy is read-only, as Qt makes the copy of a resource.
/// @return Path of the copy, empty if it could not be made
QString copyResource(const QString& dirPath, const QString& name);

/// @return Content of a file or Qt resource, empty if it cannot be read
QByteArray readFile(const QString& path);

/// @return CRC-32 of @a data as gzip and ZIP store it
quint32 crc32(const QByteArray& data);

/// @return @a data as a raw deflate stream, which is what gzip and ZIP hold
QByteArray deflate(const QByteArray& data);

/// @return One member of a gzip file, holding @a payload. A gzip file is one or more of them.
QByteArray gzipMember(const QByteArray& payload);

}  // namespace TestFixtures
