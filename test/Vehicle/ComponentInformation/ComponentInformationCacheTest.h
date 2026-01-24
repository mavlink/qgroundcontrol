#pragma once

#include "TestFixtures.h"

#include <QtCore/QString>

/// Unit test for ComponentInformationCache.
/// Uses OfflineTest since it doesn't require a vehicle connection.
class ComponentInformationCacheTest : public OfflineTest
{
    Q_OBJECT

public:
    ComponentInformationCacheTest() = default;

private slots:
    void init() override;
    void cleanup() override;

    // Cache Tests
    void _basic_test();
    void _lru_test();
    void _multi_test();

private:
    struct TmpFile {
        QString path;
        QString cacheTag;
        QString content;
        QString cachedPath;
    };

    QVector<TmpFile> _tmpFiles;

    QString _cacheDir;
    QString _tmpFilesDir;
};
