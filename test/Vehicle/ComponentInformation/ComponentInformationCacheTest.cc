#include "ComponentInformationCacheTest.h"
#include "ComponentInformationCache.h"

#include <QtCore/QStandardPaths>
#include <QtTest/QTest>

void ComponentInformationCacheTest::init()
{
    OfflineTest::init();

    _cacheDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/QGCCacheTest");
    _tmpFilesDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/QGCTestFiles");

    // Clean up any leftover directories from previous runs
    QDir(_tmpFilesDir).removeRecursively();
    QDir(_cacheDir).removeRecursively();

    // Create temp files directory and populate with test files
    QDir d(_tmpFilesDir);
    d.mkdir(_tmpFilesDir);

    _tmpFiles.clear();

    for (int i = 0; i < 30; ++i) {
        TmpFile t;
        t.content = QString::asprintf("%i", i);
        t.path = _tmpFilesDir + "/" + t.content + ".txt";
        t.cacheTag = QString::asprintf("_tag_%08i_xy", i);
        QFile f(t.path);
        if (f.open(QIODevice::WriteOnly)) {
            (void)f.write(t.content.toUtf8().constData(), t.content.toUtf8().size());
            f.close();
        } else {
            qWarning() << "Error opening file" << f.fileName();
        }
        _tmpFiles.push_back(t);
    }
}

void ComponentInformationCacheTest::cleanup()
{
    QDir(_tmpFilesDir).removeRecursively();
    QDir(_cacheDir).removeRecursively();

    OfflineTest::cleanup();
}

void ComponentInformationCacheTest::_basic_test()
{
    ComponentInformationCache cache(_cacheDir, 10);

    QDir cacheDir(_cacheDir);
    QVERIFY(cacheDir.exists());

    _tmpFiles[0].cachedPath = cache.insert(_tmpFiles[0].cacheTag, _tmpFiles[0].path);
    QGC_VERIFY_NOT_EMPTY(_tmpFiles[0].cachedPath);
    VERIFY_FILE_EXISTS(_tmpFiles[0].cachedPath);
    VERIFY_FILE_NOT_EXISTS(_tmpFiles[0].path);

    QCOMPARE_EQ(cache.access(_tmpFiles[0].cacheTag), _tmpFiles[0].cachedPath);

    QFile f(_tmpFiles[0].cachedPath);
    QVERIFY(f.open(QFile::ReadOnly | QFile::Text));
    QTextStream in(&f);
    QCOMPARE_EQ(in.readAll(), _tmpFiles[0].content);
}


void ComponentInformationCacheTest::_lru_test()
{
    ComponentInformationCache cache(_cacheDir, 3);

    auto insert = [&](int idx) {
        _tmpFiles[idx].cachedPath = cache.insert(_tmpFiles[idx].cacheTag, _tmpFiles[idx].path);
        QGC_VERIFY_NOT_EMPTY(_tmpFiles[idx].cachedPath);
    };
    insert(1);
    insert(3);
    insert(0);

    QCOMPARE_EQ(cache.access(_tmpFiles[0].cacheTag), _tmpFiles[0].cachedPath);
    QCOMPARE_EQ(cache.access(_tmpFiles[1].cacheTag), _tmpFiles[1].cachedPath);
    QCOMPARE_EQ(cache.access(_tmpFiles[3].cacheTag), _tmpFiles[3].cachedPath);

    insert(4);

    QGC_VERIFY_EMPTY(cache.access(_tmpFiles[0].cacheTag));
    QCOMPARE_EQ(cache.access(_tmpFiles[1].cacheTag), _tmpFiles[1].cachedPath);
    QCOMPARE_EQ(cache.access(_tmpFiles[3].cacheTag), _tmpFiles[3].cachedPath);
    QCOMPARE_EQ(cache.access(_tmpFiles[4].cacheTag), _tmpFiles[4].cachedPath);

    QCOMPARE_EQ(cache.access(_tmpFiles[3].cacheTag), _tmpFiles[3].cachedPath);
    QCOMPARE_EQ(cache.access(_tmpFiles[1].cacheTag), _tmpFiles[1].cachedPath);
    QCOMPARE_EQ(cache.access(_tmpFiles[3].cacheTag), _tmpFiles[3].cachedPath);

    insert(5);

    QGC_VERIFY_EMPTY(cache.access(_tmpFiles[4].cacheTag));
    QCOMPARE_EQ(cache.access(_tmpFiles[1].cacheTag), _tmpFiles[1].cachedPath);
    QCOMPARE_EQ(cache.access(_tmpFiles[3].cacheTag), _tmpFiles[3].cachedPath);
    QCOMPARE_EQ(cache.access(_tmpFiles[5].cacheTag), _tmpFiles[5].cachedPath);

    QCOMPARE_EQ(cache.access(_tmpFiles[3].cacheTag), _tmpFiles[3].cachedPath);
    insert(6);
    insert(7);

    QGC_VERIFY_EMPTY(cache.access(_tmpFiles[4].cacheTag));
    QGC_VERIFY_EMPTY(cache.access(_tmpFiles[1].cacheTag));
    QGC_VERIFY_EMPTY(cache.access(_tmpFiles[5].cacheTag));
    QCOMPARE_EQ(cache.access(_tmpFiles[3].cacheTag), _tmpFiles[3].cachedPath);
    QCOMPARE_EQ(cache.access(_tmpFiles[6].cacheTag), _tmpFiles[6].cachedPath);
    QCOMPARE_EQ(cache.access(_tmpFiles[7].cacheTag), _tmpFiles[7].cachedPath);
}

void ComponentInformationCacheTest::_multi_test()
{
    auto insert = [&](ComponentInformationCache& cache, int idx) {
        _tmpFiles[idx].cachedPath = cache.insert(_tmpFiles[idx].cacheTag, _tmpFiles[idx].path);
        QGC_VERIFY_NOT_EMPTY(_tmpFiles[idx].cachedPath);
    };

    {
        ComponentInformationCache cache(_cacheDir, 5);
        for (int i = 0; i < 5; ++i) {
            insert(cache, i);
            QCOMPARE_EQ(cache.access(_tmpFiles[i].cacheTag), _tmpFiles[i].cachedPath);
        }
        QCOMPARE_EQ(cache.access(_tmpFiles[1].cacheTag), _tmpFiles[1].cachedPath);
    }
    {
        // reduce cache size and ensure oldest entries are evicted
        ComponentInformationCache cache(_cacheDir, 3);
        QGC_VERIFY_EMPTY(cache.access(_tmpFiles[0].cacheTag));
        QCOMPARE_EQ(cache.access(_tmpFiles[1].cacheTag), _tmpFiles[1].cachedPath);
        QGC_VERIFY_EMPTY(cache.access(_tmpFiles[2].cacheTag));
        QCOMPARE_EQ(cache.access(_tmpFiles[3].cacheTag), _tmpFiles[3].cachedPath);
        QCOMPARE_EQ(cache.access(_tmpFiles[4].cacheTag), _tmpFiles[4].cachedPath);

        insert(cache, 10);
        QGC_VERIFY_EMPTY(cache.access(_tmpFiles[1].cacheTag));
        QCOMPARE_EQ(cache.access(_tmpFiles[10].cacheTag), _tmpFiles[10].cachedPath);
    }
}
