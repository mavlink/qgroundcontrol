/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include "ComponentInformationCache.h"

#include "UnitTest.h"

#include <QString>

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
private:
    void _setup();
    void _cleanup();

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

