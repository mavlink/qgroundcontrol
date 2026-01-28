#include "GeoFileIO.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonParseError>
#include <QtCore/QLoggingCategory>
#include <QtCore/QTextStream>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

Q_LOGGING_CATEGORY(GeoFileIOLog, "qgc.utilities.geo.fileio")

namespace GeoFileIO
{

// ============================================================================
// Error Formatting
// ============================================================================

QString formatLoadError(const QString &formatName, const QString &detail)
{
    return QCoreApplication::translate("GeoFileIO", "%1 file load failed. %2")
        .arg(formatName, detail);
}

QString formatSaveError(const QString &formatName, const QString &detail)
{
    return QCoreApplication::translate("GeoFileIO", "%1 file save failed. %2")
        .arg(formatName, detail);
}

QString formatNoEntitiesError(const QString &formatName, const QString &entityType)
{
    return formatLoadError(formatName,
        QCoreApplication::translate("GeoFileIO", "No valid %1 found").arg(entityType));
}

// ============================================================================
// Utility Functions
// ============================================================================

bool checkFileReadable(const QString &filePath, const QString &formatName, QString &errorString)
{
    const QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()) {
        errorString = formatLoadError(formatName,
            QCoreApplication::translate("GeoFileIO", "File not found: %1").arg(filePath));
        return false;
    }

    if (!fileInfo.isReadable()) {
        errorString = formatLoadError(formatName,
            QCoreApplication::translate("GeoFileIO", "File not readable: %1").arg(filePath));
        return false;
    }

    return true;
}

bool checkFileWritable(const QString &filePath, const QString &formatName, QString &errorString)
{
    const QFileInfo fileInfo(filePath);
    const QDir parentDir = fileInfo.absoluteDir();

    if (!parentDir.exists()) {
        errorString = formatSaveError(formatName,
            QCoreApplication::translate("GeoFileIO", "Directory does not exist: %1")
                .arg(parentDir.absolutePath()));
        return false;
    }

    // Check if we can write to the directory
    const QFileInfo dirInfo(parentDir.absolutePath());
    if (!dirInfo.isWritable()) {
        errorString = formatSaveError(formatName,
            QCoreApplication::translate("GeoFileIO", "Directory not writable: %1")
                .arg(parentDir.absolutePath()));
        return false;
    }

    return true;
}

// ============================================================================
// Text File Operations
// ============================================================================

TextResult loadText(const QString &filePath, const QString &formatName)
{
    TextResult result;

    if (!checkFileReadable(filePath, formatName, result.error)) {
        return result;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.error = formatLoadError(formatName, file.errorString());
        return result;
    }

    QTextStream stream(&file);
    result.content = stream.readAll();
    file.close();

    if (result.content.isEmpty()) {
        qCWarning(GeoFileIOLog) << "Loaded empty file:" << filePath;
    }

    result.success = true;
    return result;
}

bool saveText(const QString &filePath, const QString &content,
              const QString &formatName, QString &errorString)
{
    if (!checkFileWritable(filePath, formatName, errorString)) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorString = formatSaveError(formatName, file.errorString());
        return false;
    }

    QTextStream stream(&file);
    stream << content;
    stream.flush();

    if (stream.status() != QTextStream::Ok) {
        errorString = formatSaveError(formatName,
            QCoreApplication::translate("GeoFileIO", "Write error"));
        file.close();
        return false;
    }

    file.close();

    if (file.error() != QFileDevice::NoError) {
        errorString = formatSaveError(formatName, file.errorString());
        return false;
    }

    return true;
}

// ============================================================================
// JSON File Operations
// ============================================================================

JsonResult loadJson(const QString &filePath, const QString &formatName)
{
    JsonResult result;

    if (!checkFileReadable(filePath, formatName, result.error)) {
        return result;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = formatLoadError(formatName, file.errorString());
        return result;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    result.document = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        result.error = formatLoadError(formatName,
            QCoreApplication::translate("GeoFileIO", "JSON parse error at offset %1: %2")
                .arg(parseError.offset)
                .arg(parseError.errorString()));
        return result;
    }

    result.success = true;
    return result;
}

bool saveJson(const QString &filePath, const QJsonDocument &doc,
              const QString &formatName, QString &errorString)
{
    if (!checkFileWritable(filePath, formatName, errorString)) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorString = formatSaveError(formatName, file.errorString());
        return false;
    }

    const qint64 bytesWritten = file.write(doc.toJson(QJsonDocument::Indented));
    if (bytesWritten == -1) {
        errorString = formatSaveError(formatName, file.errorString());
        file.close();
        return false;
    }

    file.close();

    if (file.error() != QFileDevice::NoError) {
        errorString = formatSaveError(formatName, file.errorString());
        return false;
    }

    return true;
}

// ============================================================================
// XML DOM File Operations
// ============================================================================

DomResult loadDom(const QString &filePath, const QString &formatName)
{
    DomResult result;

    if (!checkFileReadable(filePath, formatName, result.error)) {
        return result;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = formatLoadError(formatName, file.errorString());
        return result;
    }

    const QDomDocument::ParseResult parseResult =
        result.document.setContent(&file, QDomDocument::ParseOption::Default);
    file.close();

    if (!parseResult) {
        result.error = formatLoadError(formatName,
            QCoreApplication::translate("GeoFileIO", "XML parse error at line %1: %2")
                .arg(parseResult.errorLine)
                .arg(parseResult.errorMessage));
        return result;
    }

    result.success = true;
    return result;
}

bool saveDom(const QString &filePath, const QDomDocument &doc,
             const QString &formatName, QString &errorString, int indent)
{
    if (!checkFileWritable(filePath, formatName, errorString)) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorString = formatSaveError(formatName, file.errorString());
        return false;
    }

    QTextStream stream(&file);
    stream << doc.toString(indent);
    stream.flush();

    if (stream.status() != QTextStream::Ok) {
        errorString = formatSaveError(formatName,
            QCoreApplication::translate("GeoFileIO", "Write error"));
        file.close();
        return false;
    }

    file.close();

    if (file.error() != QFileDevice::NoError) {
        errorString = formatSaveError(formatName, file.errorString());
        return false;
    }

    return true;
}

// ============================================================================
// XML Stream Operations
// ============================================================================

StreamReadResult openXmlStreamForRead(const QString &filePath, const QString &formatName)
{
    StreamReadResult result;

    if (!checkFileReadable(filePath, formatName, result.error)) {
        return result;
    }

    result.file = std::make_unique<QFile>(filePath);
    if (!result.file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.error = formatLoadError(formatName, result.file->errorString());
        result.file.reset();
        return result;
    }

    result.reader = std::make_unique<QXmlStreamReader>(result.file.get());
    result.success = true;
    return result;
}

StreamWriteResult openXmlStreamForWrite(const QString &filePath, const QString &formatName,
                                        bool autoFormatting, int indentSize)
{
    StreamWriteResult result;

    if (!checkFileWritable(filePath, formatName, result.error)) {
        return result;
    }

    result.file = std::make_unique<QFile>(filePath);
    if (!result.file->open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.error = formatSaveError(formatName, result.file->errorString());
        result.file.reset();
        return result;
    }

    result.writer = std::make_unique<QXmlStreamWriter>(result.file.get());
    result.writer->setAutoFormatting(autoFormatting);
    result.writer->setAutoFormattingIndent(indentSize);

    result.success = true;
    return result;
}

bool closeXmlStream(StreamWriteResult &result, const QString &formatName, QString &errorString)
{
    if (!result.writer || !result.file) {
        errorString = formatSaveError(formatName,
            QCoreApplication::translate("GeoFileIO", "Invalid stream"));
        return false;
    }

    if (result.writer->hasError()) {
        errorString = formatSaveError(formatName,
            QCoreApplication::translate("GeoFileIO", "XML write error"));
        result.file->close();
        return false;
    }

    result.file->close();

    if (result.file->error() != QFileDevice::NoError) {
        errorString = formatSaveError(formatName, result.file->errorString());
        return false;
    }

    return true;
}

} // namespace GeoFileIO
