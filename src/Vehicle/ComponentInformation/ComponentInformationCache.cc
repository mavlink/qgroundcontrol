#include "ComponentInformationCache.h"
#include "QGCFileHelper.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>
#include <QtCore/QDirIterator>
#include <QtCore/QStandardPaths>

QGC_LOGGING_CATEGORY(ComponentInformationCacheLog, "ComponentInformation.ComponentInformationCache")

ComponentInformationCache::ComponentInformationCache(const QDir& path, int maxNumFiles)
    : _path(path), _maxNumFiles(maxNumFiles)
{
    initializeDirectory();
}

ComponentInformationCache& ComponentInformationCache::defaultInstance()
{
    static ComponentInformationCache instance(defaultDirectory(), 50);
    return instance;
}

QString ComponentInformationCache::defaultDirectory()
{
    // Not "QGCCompInfoCache", where an earlier QGC keeps its entries of version 0 and removes every other one when it
    // starts
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1String("/QGCCompInfoCacheV1");
}

QString ComponentInformationCache::metaFileName(const QString& fileTag)
{
    return _path.filePath(fileTag+_metaExtension);
}

QString ComponentInformationCache::dataFileName(const QString& fileTag)
{
    return _path.filePath(fileTag+_cacheExtension);
}

QString ComponentInformationCache::access(const QString &fileTag)
{
    QFile meta(metaFileName(fileTag));
    QFile data(dataFileName(fileTag));
    if (!meta.exists() || !data.exists()) {
        qCDebug(ComponentInformationCacheLog) << "Cache miss for" << fileTag;
        return "";
    }

    qCDebug(ComponentInformationCacheLog) << "Cache hit for" << fileTag;

    // mark access
    Meta m{};
    AccessCounterType previousCounter = -1;
    if (meta.open(QIODevice::ReadWrite)) {
        if (meta.read((char*)&m, sizeof(m)) == sizeof(m)) {
            previousCounter = m.accessCounter;
            m.accessCounter = _nextAccessCounter;
            meta.seek(0);
            if (meta.write((const char*)&m, sizeof(m)) != sizeof(m)) {
                qCWarning(ComponentInformationCacheLog) << "Meta write failed" << meta.fileName() << meta.errorString();
            }
        } else {
            qCWarning(ComponentInformationCacheLog) << "Meta read failed" << meta.fileName() << meta.errorString();
        }
        meta.close();
    } else {
        qCWarning(ComponentInformationCacheLog) << "Failed to open" << meta.fileName() << meta.errorString();
    }

    _cachedFiles.remove(previousCounter);
    _cachedFiles[_nextAccessCounter] = fileTag;
    ++_nextAccessCounter;

    return data.fileName();
}

QString ComponentInformationCache::insert(const QString &fileTag, const QString &fileName)
{
    QFile meta(metaFileName(fileTag));
    QFile data(dataFileName(fileTag));
    QFile fileToCache(fileName);
    if (meta.exists() && data.exists()) {
        qCDebug(ComponentInformationCacheLog) << "Not inserting, entry already exists" << fileTag;
        fileToCache.remove();
        return data.fileName();
    }

    // A file without its counterpart, e.g. a data file whose removal failed while it was open, is not an entry:
    // access() never finds it, so it is replaced. If it is still in the way, the new file is used where it is
    if (data.exists() && !data.remove()) {
        qCWarning(ComponentInformationCacheLog)
            << "Failed to remove incomplete entry" << data.fileName() << data.errorString();
        return fileName;
    }
    _numFiles -=
        static_cast<int>(_cachedFiles.removeIf([&fileTag](const auto& entry) { return entry.value() == fileTag; }));

    // move the file to the cache location (fall back to copy+remove for cross-device)
    if (!fileToCache.rename(data.fileName())) {
        qCDebug(ComponentInformationCacheLog) << "File rename failed from:to" << fileName << data.fileName() << fileToCache.errorString();
        if (!fileToCache.copy(data.fileName())) {
            qCWarning(ComponentInformationCacheLog) << "File copy failed from:to" << fileName << data.fileName() << fileToCache.errorString();
            return "";
        }
        if (!fileToCache.remove()) {
            qCWarning(ComponentInformationCacheLog) << "File remove failed after copy for" << fileToCache.fileName() << fileToCache.errorString();
        }
    }

    // write meta data
    Meta m{};
    m.accessCounter = _nextAccessCounter;
    if (meta.open(QIODevice::WriteOnly)) {
        if (meta.write((const char*)&m, sizeof(m)) != sizeof(m)) {
            qCWarning(ComponentInformationCacheLog) << "Meta write failed" << meta.fileName() << meta.errorString();
        }
        meta.close();
    } else {
        qCWarning(ComponentInformationCacheLog) << "Failed to open" << meta.fileName() << meta.errorString();
    }

    // update internal data
    _cachedFiles[_nextAccessCounter++] = fileTag;
    ++_numFiles;

    removeOldEntries();
    return data.fileName();
}

void ComponentInformationCache::remove(const QString& fileTag)
{
    qCDebug(ComponentInformationCacheLog) << "Removing" << fileTag;
    // The meta file first: without it the entry is not found, also while its data file cannot be removed, which
    // insert() then replaces
    (void) QFile::remove(metaFileName(fileTag));
    (void) QFile::remove(dataFileName(fileTag));
    _numFiles -=
        static_cast<int>(_cachedFiles.removeIf([&fileTag](const auto& entry) { return entry.value() == fileTag; }));
}

void ComponentInformationCache::initializeDirectory()
{
    if (!QGCFileHelper::ensureDirectoryExists(_path.path())) {
        qCWarning(ComponentInformationCacheLog) << "Failed to create dir" << _path.path();
    }

    QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot;
    QDirIterator it(_path.path(), filters, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        QString path = it.next();

        if (path.endsWith(_metaExtension)) {
            QFile meta(path);
            QFile data(path.mid(0, path.length()-strlen(_metaExtension))+_cacheExtension);
            bool validationFailed = false;
            if (!data.exists()) {
                validationFailed = true;
            }

            // read meta + validate
            Meta m{};
            const uint32_t expectedMagic = m.magic;
            const uint32_t expectedVersion = m.version;
            if (meta.open(QIODevice::ReadOnly)) {
                if (meta.read((char*)&m, sizeof(m)) == sizeof(m)) {
                    if (m.magic != expectedMagic || m.version != expectedVersion) {
                        validationFailed = true;
                    }
                } else {
                    validationFailed = true;
                }
                meta.close();
            } else {
                validationFailed = true;
            }

            if (validationFailed) {
                qCWarning(ComponentInformationCacheLog) << "Validation failed, removing cache files" << path;
                meta.remove();
                data.remove();
            } else {
                // extract the tag
                QString tag = it.fileName();
                tag = tag.mid(0, tag.length()-strlen(_metaExtension));
                _cachedFiles[m.accessCounter] = tag;

                qCDebug(ComponentInformationCacheLog) << "Found cached file:counter" << meta.fileName() << m.accessCounter;

                if (m.accessCounter >= _nextAccessCounter) {
                    _nextAccessCounter = m.accessCounter + 1;
                }
            }

        } else if (!path.endsWith(_cacheExtension)) {
            QFile::remove(path);
        }
    }
    _numFiles = _cachedFiles.size();
    removeOldEntries();
}

void ComponentInformationCache::removeOldEntries()
{
    while (_numFiles > _maxNumFiles) {
        auto iter = _cachedFiles.begin();
        QFile meta(metaFileName(iter.value()));
        QFile data(dataFileName(iter.value()));
        qCDebug(ComponentInformationCacheLog) << "Removing cache entry num:counter:file" << _numFiles << iter.key() << iter.value();
        meta.remove();
        data.remove();

        _cachedFiles.erase(iter);
        --_numFiles;
    }
}
