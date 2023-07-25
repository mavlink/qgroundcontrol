/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "ComponentInformationCacheTest.h"


ComponentInformationCacheTest::ComponentInformationCacheTest()
{
    _cacheDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/QGCCacheTest");
    _tmpFilesDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/QGCTestFiles");
    _cleanup();
}

void ComponentInformationCacheTest::_setup()
{
    QDir d(_tmpFilesDir);
    d.mkdir(_tmpFilesDir);

    _tmpFiles.clear();

    for (int i = 0; i < 30; ++i) {
        TmpFile t;
        t.content = QString::asprintf("%i", i);
        t.path = _tmpFilesDir + "/" + t.content + ".txt";
        t.cacheTag = QString::asprintf("_tag_%08i_xy", i);
        QFile f(t.path);
        f.open(QIODevice::WriteOnly);
        f.write(t.content.toUtf8().constData(), t.content.toUtf8().size());
        f.close();
        _tmpFiles.push_back(t);
    }
}

void ComponentInformationCacheTest::_cleanup()
{
    QDir t(_tmpFilesDir);
    t.removeRecursively();
    QDir d(_cacheDir);
    d.removeRecursively();
}

void ComponentInformationCacheTest::_basic_test()
{
    _setup();
    ComponentInformationCache cache(_cacheDir, 10);

    QDir cacheDir(_cacheDir);
    QVERIFY(cacheDir.exists());

    _tmpFiles[0].cachedPath = cache.insert(_tmpFiles[0].cacheTag, _tmpFiles[0].path);
    QVERIFY(!_tmpFiles[0].cachedPath.isEmpty());
    QVERIFY(QFile(_tmpFiles[0].cachedPath).exists());
    QVERIFY(!QFile(_tmpFiles[0].path).exists());

    QVERIFY(cache.access(_tmpFiles[0].cacheTag) == _tmpFiles[0].cachedPath);

    QFile f(_tmpFiles[0].cachedPath);
    QVERIFY(f.open(QFile::ReadOnly | QFile::Text));
    QTextStream in(&f);
    QVERIFY(in.readAll() == _tmpFiles[0].content);

    _cleanup();
}


void ComponentInformationCacheTest::_lru_test()
{
    _setup();
    ComponentInformationCache cache(_cacheDir, 3);

    auto insert = [&](int idx) {
        _tmpFiles[idx].cachedPath = cache.insert(_tmpFiles[idx].cacheTag, _tmpFiles[idx].path);
        QVERIFY(!_tmpFiles[idx].cachedPath.isEmpty());
    };
    insert(1);
    insert(3);
    insert(0);

    QVERIFY(cache.access(_tmpFiles[0].cacheTag) == _tmpFiles[0].cachedPath);
    QVERIFY(cache.access(_tmpFiles[1].cacheTag) == _tmpFiles[1].cachedPath);
    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);

    insert(4);

    QVERIFY(cache.access(_tmpFiles[0].cacheTag) == "");
    QVERIFY(cache.access(_tmpFiles[1].cacheTag) == _tmpFiles[1].cachedPath);
    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
    QVERIFY(cache.access(_tmpFiles[4].cacheTag) == _tmpFiles[4].cachedPath);

    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
    QVERIFY(cache.access(_tmpFiles[1].cacheTag) == _tmpFiles[1].cachedPath);
    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);

    insert(5);

    QVERIFY(cache.access(_tmpFiles[4].cacheTag) == "");
    QVERIFY(cache.access(_tmpFiles[1].cacheTag) == _tmpFiles[1].cachedPath);
    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
    QVERIFY(cache.access(_tmpFiles[5].cacheTag) == _tmpFiles[5].cachedPath);

    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
    insert(6);
    insert(7);

    QVERIFY(cache.access(_tmpFiles[4].cacheTag) == "");
    QVERIFY(cache.access(_tmpFiles[1].cacheTag) == "");
    QVERIFY(cache.access(_tmpFiles[5].cacheTag) == "");
    QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
    QVERIFY(cache.access(_tmpFiles[6].cacheTag) == _tmpFiles[6].cachedPath);
    QVERIFY(cache.access(_tmpFiles[7].cacheTag) == _tmpFiles[7].cachedPath);

    _cleanup();
}

void ComponentInformationCacheTest::_multi_test()
{
    _setup();

    auto insert = [&](ComponentInformationCache& cache, int idx) {
        _tmpFiles[idx].cachedPath = cache.insert(_tmpFiles[idx].cacheTag, _tmpFiles[idx].path);
        QVERIFY(!_tmpFiles[idx].cachedPath.isEmpty());
    };

    {
        ComponentInformationCache cache(_cacheDir, 5);
        for (int i = 0; i < 5; ++i) {
            insert(cache, i);
            QVERIFY(cache.access(_tmpFiles[i].cacheTag) == _tmpFiles[i].cachedPath);
        }
        QVERIFY(cache.access(_tmpFiles[1].cacheTag) == _tmpFiles[1].cachedPath);
    }
    {
        // reduce cache size and ensure oldest entries are evicted
        ComponentInformationCache cache(_cacheDir, 3);
        QVERIFY(cache.access(_tmpFiles[0].cacheTag) == "");
        QVERIFY(cache.access(_tmpFiles[1].cacheTag) == _tmpFiles[1].cachedPath);
        QVERIFY(cache.access(_tmpFiles[2].cacheTag) == "");
        QVERIFY(cache.access(_tmpFiles[3].cacheTag) == _tmpFiles[3].cachedPath);
        QVERIFY(cache.access(_tmpFiles[4].cacheTag) == _tmpFiles[4].cachedPath);

        insert(cache, 10);
        QVERIFY(cache.access(_tmpFiles[1].cacheTag) == "");
        QVERIFY(cache.access(_tmpFiles[10].cacheTag) == _tmpFiles[10].cachedPath);
    }

    _cleanup();
}
