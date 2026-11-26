#pragma once

#include <QtCore/QString>
#include <QtCore/QDir>
#include <QtCore/QMap>

/**
 * Simple file cache with a maximum number of files and LRU retention policy based on last access
 * Notes:
 * - fileTag defines the cache keys and the format is up to the user
 * - only one instance per directory must exist
 * - not thread-safe
 */
class ComponentInformationCache : public QObject
{
    Q_OBJECT
public:
    ComponentInformationCache(const QDir& path, int maxNumFiles);

    static ComponentInformationCache& defaultInstance();

    /// Directory of defaultInstance()
    static QString defaultDirectory();

    /**
     * Try to access a file and set the access counter
     * @param fileTag
     * @return empty string if not found, or file path
     */
    QString access(const QString& fileTag);

    /**
     * Insert a file into the cache & remove old files if there's too many.
     * @param fileTag
     * @param fileName file to insert, will be moved (or deleted if already exists)
     * @return cached file name if inserted or already exists, "" on error. @a fileName itself, which is then not
     *         cached, if a leftover file of that tag is in the way and cannot be removed
     */
    QString insert(const QString &fileTag, const QString& fileName);

    /// Remove the entry of @a fileTag, such as one whose file cannot be read, if there is one
    void remove(const QString& fileTag);

private:

    static constexpr const char* _metaExtension = ".meta";
    static constexpr const char* _cacheExtension = ".cache";

    using AccessCounterType = uint64_t;

    struct Meta {
        uint32_t magic{0x9a9cad0e};
        /// 1: the file is kept as it was downloaded, possibly compressed. An entry of another version is removed.
        /// An older QGC, which takes only version 0 (decompressed), keeps its entries in another directory
        uint32_t version{1};
        AccessCounterType accessCounter{0};
    };

    void initializeDirectory();
    void removeOldEntries();

    QString metaFileName(const QString& fileTag);
    QString dataFileName(const QString& fileTag);

    const QDir _path;
    const int _maxNumFiles;

    AccessCounterType _nextAccessCounter{0};
    int _numFiles{0};
    QMap<AccessCounterType, QString> _cachedFiles;
};
