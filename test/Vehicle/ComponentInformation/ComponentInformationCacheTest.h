#pragma once

#include <QtCore/QString>

#include "UnitTest.h"

class ComponentInformationCacheTest : public UnitTest
{
    Q_OBJECT

public:
    ComponentInformationCacheTest();
    virtual ~ComponentInformationCacheTest() = default;

private slots:
    void _basic_test();
    void _lru_test();
    void _multi_test();
    UT_PARAMETERIZED_TEST(_version_test);

private:
    void _setup();
    void _cleanup();

    struct TmpFile
    {
        QString path;
        QString cacheTag;
        QString content;
        QString cachedPath;
    };

    QVector<TmpFile> _tmpFiles;

    QString _cacheDir;
    QString _tmpFilesDir;
};
