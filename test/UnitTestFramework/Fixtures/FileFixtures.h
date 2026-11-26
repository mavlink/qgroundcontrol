#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

/// @file
/// @brief Files and file content that tests of several areas make

namespace TestFixtures {

/// Write a file, which is created or replaced
/// @return true if all of @a data was written
bool writeFile(const QString& path, const QByteArray& data);

/// Write a file that its owner may run: a script that stands in for a program
/// @return true if all of @a script was written and the file made executable
bool writeExecutable(const QString& path, const QByteArray& script);

/// @return Content of a file or Qt resource, empty if it cannot be read
QByteArray readFile(const QString& path);

/// Create a symbolic link to a file or a directory. Unlike QFile::link(), which makes a shortcut on Windows, this
/// makes a link that the file system follows. Windows lets only an administrator, or any user in developer mode,
/// create one
/// @return true if the link was created
bool createSymbolicLink(const QString& targetPath, const QString& linkPath);

/// @return CRC-32 of @a data as gzip and ZIP store it
quint32 crc32(const QByteArray& data);

/// @return @a data as a raw deflate stream, which is what gzip and ZIP hold
QByteArray deflate(const QByteArray& data);

/// @return One member of a gzip file, holding @a payload. A gzip file is one or more of them.
QByteArray gzipMember(const QByteArray& payload);

}  // namespace TestFixtures
