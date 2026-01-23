#pragma once

#include <QtCore/QJsonDocument>
#include <QtCore/QString>
#include <QtXml/QDomDocument>

#include <memory>

class QFile;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace GeoFileIO
{

/// Format standardized load error message
/// @param formatName Format name (e.g., "KML", "GPX", "GeoJSON")
/// @param detail Specific error detail
/// @return Formatted error string suitable for user display
QString formatLoadError(const QString &formatName, const QString &detail);

/// Format standardized save error message
QString formatSaveError(const QString &formatName, const QString &detail);

/// Format "no entities found" error message
/// @param formatName Format name (e.g., "KML", "GPX")
/// @param entityType Entity type (e.g., "polygons", "polylines", "points")
/// @return Formatted error message
QString formatNoEntitiesError(const QString &formatName, const QString &entityType);

// ============================================================================
// Text File Operations (for WKT, OpenAir, etc.)
// ============================================================================

struct TextResult {
    bool success = false;
    QString content;
    QString error;
};

/// Load entire text file content
/// @param filePath Path to text file
/// @param formatName Format name for error messages
/// @return Result with content or error
TextResult loadText(const QString &filePath, const QString &formatName);

/// Save text content to file
/// @param filePath Path to save to
/// @param content Text content to write
/// @param formatName Format name for error messages
/// @param errorString Output error message on failure
/// @return true on success
bool saveText(const QString &filePath, const QString &content,
              const QString &formatName, QString &errorString);

// ============================================================================
// JSON File Operations (for GeoJSON)
// ============================================================================

struct JsonResult {
    bool success = false;
    QJsonDocument document;
    QString error;
};

/// Load and parse JSON file
/// @param filePath Path to JSON file
/// @param formatName Format name for error messages
/// @return Result with parsed document or error
JsonResult loadJson(const QString &filePath, const QString &formatName);

/// Save JSON document to file
/// @param filePath Path to save to
/// @param doc JSON document to write
/// @param formatName Format name for error messages
/// @param errorString Output error message on failure
/// @return true on success
bool saveJson(const QString &filePath, const QJsonDocument &doc,
              const QString &formatName, QString &errorString);

// ============================================================================
// XML DOM File Operations (for KML)
// ============================================================================

struct DomResult {
    bool success = false;
    QDomDocument document;
    QString error;
};

/// Load and parse XML DOM file
/// @param filePath Path to XML file
/// @param formatName Format name for error messages
/// @return Result with parsed document or error
DomResult loadDom(const QString &filePath, const QString &formatName);

/// Save XML DOM document to file
/// @param filePath Path to save to
/// @param doc DOM document to write
/// @param formatName Format name for error messages
/// @param errorString Output error message on failure
/// @param indent Indentation level (default 2)
/// @return true on success
bool saveDom(const QString &filePath, const QDomDocument &doc,
             const QString &formatName, QString &errorString, int indent = 2);

// ============================================================================
// XML Stream Operations (for GPX - more efficient for large files)
// ============================================================================

struct StreamReadResult {
    bool success = false;
    std::unique_ptr<QFile> file;
    std::unique_ptr<QXmlStreamReader> reader;
    QString error;
};

/// Open file and create XML stream reader
/// @param filePath Path to XML file
/// @param formatName Format name for error messages
/// @return Result with file and reader, or error
StreamReadResult openXmlStreamForRead(const QString &filePath, const QString &formatName);

struct StreamWriteResult {
    bool success = false;
    std::unique_ptr<QFile> file;
    std::unique_ptr<QXmlStreamWriter> writer;
    QString error;
};

/// Create file and XML stream writer
/// @param filePath Path to create
/// @param formatName Format name for error messages
/// @param autoFormatting Enable auto-formatting (default true)
/// @param indentSize Indentation size (default 2)
/// @return Result with file and writer, or error
StreamWriteResult openXmlStreamForWrite(const QString &filePath, const QString &formatName,
                                        bool autoFormatting = true, int indentSize = 2);

/// Close XML stream writer and verify success
/// @param result Stream write result to close
/// @param formatName Format name for error messages
/// @param errorString Output error message on failure
/// @return true on success
bool closeXmlStream(StreamWriteResult &result, const QString &formatName, QString &errorString);

// ============================================================================
// Utility Functions
// ============================================================================

/// Check if file exists and is readable
/// @param filePath Path to check
/// @param formatName Format name for error messages
/// @param errorString Output error message if not accessible
/// @return true if file exists and is readable
bool checkFileReadable(const QString &filePath, const QString &formatName, QString &errorString);

/// Check if file path is writable (parent directory exists and is writable)
/// @param filePath Path to check
/// @param formatName Format name for error messages
/// @param errorString Output error message if not writable
/// @return true if location is writable
bool checkFileWritable(const QString &filePath, const QString &formatName, QString &errorString);

// ============================================================================
// Output Clearing Helpers (reduce boilerplate in load functions)
// ============================================================================

/// Clear error string and output list
template<typename T>
void clearOutputs(QString &errorString, QList<T> &items)
{
    errorString.clear();
    items.clear();
}

/// Clear error string and two output lists
template<typename T, typename U>
void clearOutputs(QString &errorString, QList<T> &items1, QList<U> &items2)
{
    errorString.clear();
    items1.clear();
    items2.clear();
}

} // namespace GeoFileIO
