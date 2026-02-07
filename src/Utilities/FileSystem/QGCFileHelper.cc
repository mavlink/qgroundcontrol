#include "QGCFileHelper.h"

#include "QGCCompression.h"
#include "QGCDecompressDevice.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSaveFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QStorageInfo>
#include <QtCore/QTemporaryFile>
#include <QtCore/QUrl>

Q_STATIC_LOGGING_CATEGORY(QGCFileHelperLog, "Utilities.QGCFileHelper")

namespace QGCFileHelper {

QByteArray readFile(const QString &filePath, QString *errorString, qint64 maxBytes)
{
    if (filePath.isEmpty()) {
        if (errorString != nullptr) {
            *errorString = QObject::tr("File path is empty");
        }
        return {};
    }

    if (QGCCompression::isCompressedFile(filePath)) {
        QGCDecompressDevice decompressor(filePath);
        if (!decompressor.open(QIODevice::ReadOnly)) {
            if (errorString != nullptr) {
                *errorString = QObject::tr("Failed to open compressed file: %1").arg(filePath);
            }
            return {};
        }

        QByteArray data;
        if (maxBytes > 0) {
            data = decompressor.read(maxBytes);
        } else {
            data = decompressor.readAll();
        }
        decompressor.close();
        return data;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorString != nullptr) {
            *errorString = QObject::tr("Failed to open file: %1 - %2").arg(filePath, file.errorString());
        }
        return {};
    }

    QByteArray data;
    if (maxBytes > 0) {
        data = file.read(maxBytes);
    } else {
        data = file.readAll();
    }
    file.close();
    return data;
}

size_t optimalBufferSize(const QString &path)
{
    // Cache the result - only compute once per process
    static size_t cachedSize = 0;
    if (cachedSize != 0) {
        return cachedSize;
    }

    qint64 blockSize = 0;

    // Try to get block size from specified path
    if (!path.isEmpty()) {
        QStorageInfo storage(path);
        if (storage.isValid()) {
            blockSize = storage.blockSize();
        }
    }

    // Fallback: use root filesystem
    if (blockSize <= 0) {
        blockSize = QStorageInfo::root().blockSize();
    }

    if (blockSize <= 0) {
        cachedSize = kBufferSizeDefault;
    } else {
        // Use 16 blocks, clamped to min/max bounds
        // Typical: 4KB block × 16 = 64KB
        cachedSize = static_cast<size_t>(
            qBound(static_cast<qint64>(kBufferSizeMin),
                   blockSize * 16,
                   static_cast<qint64>(kBufferSizeMax))
        );
    }

    return cachedSize;
}

bool exists(const QString &path)
{
    return isQtResource(path) || QFile::exists(path);
}

QString joinPath(const QString &dir, const QString &name)
{
    if (dir.isEmpty()) {
        return name;
    }
    if (dir.endsWith(QLatin1Char('/'))) {
        return dir + name;
    }
    return dir + QLatin1Char('/') + name;
}

bool ensureDirectoryExists(const QString &path)
{
    QDir dir(path);
    if (dir.exists()) {
        return true;
    }
    return dir.mkpath(path);
}

bool ensureParentExists(const QString &filePath)
{
    return ensureDirectoryExists(QFileInfo(filePath).absolutePath());
}

bool copyDirectoryRecursively(const QString &sourcePath, const QString &destPath)
{
    QDir sourceDir(sourcePath);
    if (!sourceDir.exists()) {
        qCWarning(QGCFileHelperLog) << "Source directory doesn't exist:" << sourcePath;
        return false;
    }

    if (!ensureDirectoryExists(destPath)) {
        qCWarning(QGCFileHelperLog) << "Failed to create destination directory:" << destPath;
        return false;
    }

    // Copy files
    const QStringList files = sourceDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QString &fileName : files) {
        const QString srcFilePath = joinPath(sourcePath, fileName);
        const QString dstFilePath = joinPath(destPath, fileName);
        if (!QFile::copy(srcFilePath, dstFilePath)) {
            qCWarning(QGCFileHelperLog) << "Failed to copy file:" << srcFilePath << "to" << dstFilePath;
            return false;
        }
    }

    // Recursively copy subdirectories
    const QStringList dirs = sourceDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &dirName : dirs) {
        const QString srcSubDir = joinPath(sourcePath, dirName);
        const QString dstSubDir = joinPath(destPath, dirName);
        if (!copyDirectoryRecursively(srcSubDir, dstSubDir)) {
            return false;
        }
    }

    return true;
}

bool moveFileOrCopy(const QString &sourcePath, const QString &destPath)
{
    if (sourcePath.isEmpty() || destPath.isEmpty()) {
        qCWarning(QGCFileHelperLog) << "moveFileOrCopy: empty path";
        return false;
    }

    // Try atomic rename first (only works on same filesystem)
    if (QFile::rename(sourcePath, destPath)) {
        return true;
    }

    // Rename failed - fall back to copy + delete
    qCDebug(QGCFileHelperLog) << "moveFileOrCopy: rename failed, using copy for:" << sourcePath;

    const bool isDir = QFileInfo(sourcePath).isDir();
    bool copySuccess = false;

    if (isDir) {
        copySuccess = copyDirectoryRecursively(sourcePath, destPath);
        if (copySuccess) {
            QDir(sourcePath).removeRecursively();
        }
    } else {
        copySuccess = QFile::copy(sourcePath, destPath);
        if (copySuccess) {
            QFile::remove(sourcePath);
        }
    }

    if (!copySuccess) {
        qCWarning(QGCFileHelperLog) << "moveFileOrCopy: failed to move:" << sourcePath << "to" << destPath;
    }

    return copySuccess;
}

bool atomicWrite(const QString &filePath, const QByteArray &data)
{
    if (filePath.isEmpty()) {
        qCWarning(QGCFileHelperLog) << "atomicWrite: file path is empty";
        return false;
    }

    // Ensure parent directory exists
    if (!ensureParentExists(filePath)) {
        qCWarning(QGCFileHelperLog) << "atomicWrite: failed to create parent directory for:" << filePath;
        return false;
    }

    // QSaveFile writes to temp file, then atomically renames on commit()
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(QGCFileHelperLog) << "atomicWrite: failed to open:" << filePath
                                    << "-" << file.errorString();
        return false;
    }

    if (file.write(data) != data.size()) {
        qCWarning(QGCFileHelperLog) << "atomicWrite: write failed:" << file.errorString();
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        qCWarning(QGCFileHelperLog) << "atomicWrite: commit failed:" << file.errorString();
        return false;
    }

    return true;
}

qint64 availableDiskSpace(const QString &path)
{
    if (path.isEmpty()) {
        return -1;
    }

    // Get storage info for the path
    QStorageInfo storage(path);
    if (!storage.isValid()) {
        // Try parent directory if path doesn't exist yet
        storage = QStorageInfo(QFileInfo(path).absolutePath());
    }

    if (!storage.isValid()) {
        qCDebug(QGCFileHelperLog) << "availableDiskSpace: cannot determine storage for:" << path;
        return -1;
    }

    return storage.bytesAvailable();
}

bool hasSufficientDiskSpace(const QString &path, qint64 requiredBytes, double margin)
{
    if (requiredBytes <= 0) {
        return true;  // Nothing to check or unknown size
    }

    const qint64 bytesAvailable = availableDiskSpace(path);
    if (bytesAvailable < 0) {
        qCDebug(QGCFileHelperLog) << "hasSufficientDiskSpace: cannot determine disk space, proceeding anyway";
        return true;  // Proceed if we can't determine space
    }

    const qint64 bytesRequired = static_cast<qint64>(static_cast<double>(requiredBytes) * margin);

    if (bytesAvailable < bytesRequired) {
        const int marginPercent = static_cast<int>((margin - 1.0) * 100);
        qCWarning(QGCFileHelperLog) << "Insufficient disk space:"
                                    << "required" << bytesRequired << "bytes"
                                    << "(" << requiredBytes << "+" << marginPercent << "% margin)"
                                    << "available" << bytesAvailable << "bytes";
        return false;
    }

    return true;
}

// ============================================================================
// URL/Path Utilities
// ============================================================================

QString toLocalPath(const QString &urlOrPath)
{
    if (urlOrPath.isEmpty()) {
        return urlOrPath;
    }

    // Already a Qt resource path in :/ format (qrc:// URLs need conversion below)
    if (urlOrPath.startsWith(QLatin1String(":/"))) {
        return urlOrPath;
    }

    // Check if it's a URL (handles qrc:// -> :/ conversion)
    QUrl url(urlOrPath);
    if (url.isValid() && !url.scheme().isEmpty()) {
        return toLocalPath(url);
    }

    // Plain path - return as-is
    return urlOrPath;
}

QString toLocalPath(const QUrl &url)
{
    if (!url.isValid()) {
        return QString();
    }

    const QString scheme = url.scheme().toLower();

    // file:// URL
    if (scheme == QLatin1String("file")) {
        return url.toLocalFile();
    }

    // qrc:// URL -> :/ resource path
    if (scheme == QLatin1String("qrc")) {
        QString path = url.path();
        if (!path.startsWith(QLatin1Char('/'))) {
            path.prepend(QLatin1Char('/'));
        }
        return QLatin1String(":/") + path.mid(1);  // qrc:/foo -> :/foo
    }

    // No scheme or unknown scheme - might be a path
    if (scheme.isEmpty()) {
        return url.path();
    }

    // Network or other URL - return the URL string
    qCDebug(QGCFileHelperLog) << "toLocalPath: URL scheme not supported for local access:" << scheme;
    return url.toString();
}

bool isLocalPath(const QString &urlOrPath)
{
    if (urlOrPath.isEmpty()) {
        return false;
    }

    // Qt resource paths are "local" (accessible via QFile)
    if (isQtResource(urlOrPath)) {
        return true;
    }

    // Check if it looks like a URL
    QUrl url(urlOrPath);
    if (url.isValid() && !url.scheme().isEmpty()) {
        const QString scheme = url.scheme().toLower();
        return scheme == QLatin1String("file") ||
               scheme == QLatin1String("qrc");
    }

    // Plain filesystem path
    return true;
}

bool isQtResource(const QString &path)
{
    return path.startsWith(QLatin1String(":/")) ||
           path.startsWith(QLatin1String("qrc:/"), Qt::CaseInsensitive);
}

// ============================================================================
// Checksum Utilities
// ============================================================================

QString computeFileHash(const QString &filePath, QCryptographicHash::Algorithm algorithm)
{
    if (filePath.isEmpty()) {
        qCWarning(QGCFileHelperLog) << "computeFileHash: empty file path";
        return QString();
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(QGCFileHelperLog) << "computeFileHash: failed to open file:" << filePath
                                    << "-" << file.errorString();
        return QString();
    }

    QCryptographicHash hash(algorithm);
    if (!hash.addData(&file)) {
        qCWarning(QGCFileHelperLog) << "computeFileHash: failed to read file:" << filePath;
        return QString();
    }

    return QString::fromLatin1(hash.result().toHex());
}

QString computeDecompressedFileHash(const QString &filePath, QCryptographicHash::Algorithm algorithm)
{
    if (filePath.isEmpty()) {
        qCWarning(QGCFileHelperLog) << "computeDecompressedFileHash: empty file path";
        return {};
    }

    if (!QGCCompression::isCompressedFile(filePath)) {
        return computeFileHash(filePath, algorithm);
    }

    QGCDecompressDevice decompressor(filePath);
    if (!decompressor.open(QIODevice::ReadOnly)) {
        qCWarning(QGCFileHelperLog) << "computeDecompressedFileHash: failed to open:" << filePath;
        return {};
    }

    QCryptographicHash hash(algorithm);
    constexpr qint64 chunkSize = 65536;
    QByteArray buffer;

    while (!decompressor.atEnd()) {
        buffer = decompressor.read(chunkSize);
        if (buffer.isEmpty() && !decompressor.atEnd()) {
            qCWarning(QGCFileHelperLog) << "computeDecompressedFileHash: read error";
            return {};
        }
        hash.addData(buffer);
    }

    decompressor.close();
    return QString::fromLatin1(hash.result().toHex());
}

QString computeHash(const QByteArray &data, QCryptographicHash::Algorithm algorithm)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, algorithm).toHex());
}

bool verifyFileHash(const QString &filePath, const QString &expectedHash,
                    QCryptographicHash::Algorithm algorithm)
{
    if (expectedHash.isEmpty()) {
        qCWarning(QGCFileHelperLog) << "verifyFileHash: empty expected hash";
        return false;
    }

    const QString actualHash = computeFileHash(filePath, algorithm);
    if (actualHash.isEmpty()) {
        return false;  // Error already logged
    }

    const bool match = (actualHash.compare(expectedHash, Qt::CaseInsensitive) == 0);
    if (!match) {
        qCWarning(QGCFileHelperLog) << "verifyFileHash: hash mismatch for" << filePath
                                    << "- expected:" << expectedHash.left(16) << "..."
                                    << "actual:" << actualHash.left(16) << "...";
    }
    return match;
}

QString hashAlgorithmName(QCryptographicHash::Algorithm algorithm)
{
    switch (algorithm) {
    case QCryptographicHash::Md4:        return QStringLiteral("MD4");
    case QCryptographicHash::Md5:        return QStringLiteral("MD5");
    case QCryptographicHash::Sha1:       return QStringLiteral("SHA-1");
    case QCryptographicHash::Sha224:     return QStringLiteral("SHA-224");
    case QCryptographicHash::Sha256:     return QStringLiteral("SHA-256");
    case QCryptographicHash::Sha384:     return QStringLiteral("SHA-384");
    case QCryptographicHash::Sha512:     return QStringLiteral("SHA-512");
    case QCryptographicHash::Sha3_224:   return QStringLiteral("SHA3-224");
    case QCryptographicHash::Sha3_256:   return QStringLiteral("SHA3-256");
    case QCryptographicHash::Sha3_384:   return QStringLiteral("SHA3-384");
    case QCryptographicHash::Sha3_512:   return QStringLiteral("SHA3-512");
    case QCryptographicHash::Blake2b_160: return QStringLiteral("BLAKE2b-160");
    case QCryptographicHash::Blake2b_256: return QStringLiteral("BLAKE2b-256");
    case QCryptographicHash::Blake2b_384: return QStringLiteral("BLAKE2b-384");
    case QCryptographicHash::Blake2b_512: return QStringLiteral("BLAKE2b-512");
    case QCryptographicHash::Blake2s_128: return QStringLiteral("BLAKE2s-128");
    case QCryptographicHash::Blake2s_160: return QStringLiteral("BLAKE2s-160");
    case QCryptographicHash::Blake2s_224: return QStringLiteral("BLAKE2s-224");
    case QCryptographicHash::Blake2s_256: return QStringLiteral("BLAKE2s-256");
    default: return QStringLiteral("Unknown");
    }
}

// ============================================================================
// Temporary File Utilities
// ============================================================================

namespace {

/// Normalize template name for QTemporaryFile
/// Ensures the template has a XXXXXX placeholder and uses default if empty
QString normalizeTemplateName(const QString &templateName)
{
    QString name = templateName.isEmpty() ? QStringLiteral("qgc_XXXXXX") : templateName;

    if (!name.contains(QLatin1String("XXXXXX"))) {
        const qsizetype dotPos = name.lastIndexOf(QLatin1Char('.'));
        if (dotPos > 0) {
            name.insert(dotPos, QLatin1String("_XXXXXX"));
        } else {
            name.append(QLatin1String("_XXXXXX"));
        }
    }

    return name;
}

} // namespace

QString tempDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
}

QString uniqueTempPath(const QString &templateName)
{
    const QString name = normalizeTemplateName(templateName);

    QTemporaryFile temp(tempDirectory() + QLatin1Char('/') + name);
    if (!temp.open()) {
        qCWarning(QGCFileHelperLog) << "uniqueTempPath: failed to create temp file";
        return {};
    }

    // Get the path before QTemporaryFile closes and removes it
    const QString path = temp.fileName();
    temp.close();
    QFile::remove(path);  // Remove so caller can create it themselves

    return path;
}

std::unique_ptr<QTemporaryFile> createTempFile(const QByteArray &data, const QString &templateName)
{
    const QString name = normalizeTemplateName(templateName);

    auto temp = std::make_unique<QTemporaryFile>(tempDirectory() + QLatin1Char('/') + name);
    if (!temp->open()) {
        qCWarning(QGCFileHelperLog) << "createTempFile: failed to create temp file";
        return nullptr;
    }

    if (temp->write(data) != data.size()) {
        qCWarning(QGCFileHelperLog) << "createTempFile: failed to write data";
        return nullptr;
    }

    // Seek to beginning for reading
    temp->seek(0);

    qCDebug(QGCFileHelperLog) << "createTempFile: created" << temp->fileName()
                              << "with" << data.size() << "bytes";
    return temp;
}

std::unique_ptr<QTemporaryFile> createTempCopy(const QString &sourcePath, const QString &templateName)
{
    if (sourcePath.isEmpty()) {
        qCWarning(QGCFileHelperLog) << "createTempCopy: source path is empty";
        return nullptr;
    }

    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        qCWarning(QGCFileHelperLog) << "createTempCopy: failed to open source:" << sourcePath
                                    << "-" << source.errorString();
        return nullptr;
    }

    QString name = templateName;
    if (name.isEmpty()) {
        // Use source filename as base
        name = QFileInfo(sourcePath).fileName() + QLatin1String("_XXXXXX");
    }

    auto temp = createTempFile(source.readAll(), name);
    if (temp) {
        qCDebug(QGCFileHelperLog) << "createTempCopy: copied" << sourcePath << "to" << temp->fileName();
    }
    return temp;
}

bool replaceFileFromTemp(QTemporaryFile *tempFile, const QString &targetPath, const QString &backupPath)
{
    if (!tempFile || !tempFile->isOpen()) {
        qCWarning(QGCFileHelperLog) << "replaceFileFromTemp: temp file is null or not open";
        return false;
    }

    if (targetPath.isEmpty()) {
        qCWarning(QGCFileHelperLog) << "replaceFileFromTemp: target path is empty";
        return false;
    }

    // Ensure parent directory exists
    if (!ensureParentExists(targetPath)) {
        qCWarning(QGCFileHelperLog) << "replaceFileFromTemp: failed to create parent directory";
        return false;
    }

    // Flush and close temp file
    tempFile->flush();

    // Create backup of existing file if requested
    if (!backupPath.isEmpty() && QFile::exists(targetPath)) {
        QFile::remove(backupPath);
        if (!QFile::copy(targetPath, backupPath)) {
            qCWarning(QGCFileHelperLog) << "replaceFileFromTemp: failed to create backup";
            return false;
        }
        qCDebug(QGCFileHelperLog) << "replaceFileFromTemp: backed up to" << backupPath;
    }

    // Remove existing target
    QFile::remove(targetPath);

    // Keep temp file around (don't auto-remove) and rename it
    tempFile->setAutoRemove(false);
    const QString tempPath = tempFile->fileName();
    tempFile->close();

    if (!moveFileOrCopy(tempPath, targetPath)) {
        qCWarning(QGCFileHelperLog) << "replaceFileFromTemp: failed to move temp file to target";
        QFile::remove(tempPath);  // Clean up on failure
        return false;
    }

    qCDebug(QGCFileHelperLog) << "replaceFileFromTemp: replaced" << targetPath;
    return true;
}

} // namespace QGCFileHelper
