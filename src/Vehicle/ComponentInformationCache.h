/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCLoggingCategory.h"

#include <QString>
#include <QDir>
#include <QMap>

#include <cstdint>

Q_DECLARE_LOGGING_CATEGORY(ComponentInformationCacheLog)

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
     * @return cached file name if inserted or already exists, "" on error
     */
    QString insert(const QString &fileTag, const QString& fileName);

private:

    static constexpr const char* _metaExtension = ".meta";
    static constexpr const char* _cacheExtension = ".cache";

    using AccessCounterType = uint64_t;

    struct Meta {
        uint32_t magic{0x9a9cad0e};
        uint32_t version{0};
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
