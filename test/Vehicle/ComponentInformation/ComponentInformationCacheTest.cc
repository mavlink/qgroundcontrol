#include "ComponentInformationCacheTest.h"

#include <cstring>

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUuid>

#include "ComponentInformationCache.h"
#include "FileFixtures.h"
#include "UnitTest.h"

ComponentInformationCacheTest::ComponentInformationCacheTest()
{
    const QString uniqueSuffix = QStringLiteral("%1_%2")
                                     .arg(QCoreApplication::applicationPid())
                                     .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QString tempRoot = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    _cacheDir = tempRoot + QLatin1String("/QGCCacheTest_") + uniqueSuffix;
    _tmpFilesDir = tempRoot + QLatin1String("/QGCTestFiles_") + uniqueSuffix;
    _cleanup();
}

void ComponentInformationCacheTest::_setup()
{
    QDir d;
    QVERIFY(d.mkpath(_tmpFilesDir));
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
    QCOMPARE(TestFixtures::readFile(_tmpFiles[0].cachedPath), _tmpFiles[0].content.toUtf8());
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

namespace {

/// The meta file of a cache entry, as ComponentInformationCache writes it
struct CacheTestMeta
{
    uint32_t magic = 0x9a9cad0e;
    uint32_t version = 0;
    uint64_t accessCounter = 0;
};

}  // namespace

void ComponentInformationCacheTest::_version_test_data()
{
    QTest::addColumn<uint>("version");
    QTest::addColumn<bool>("kept");

    // An earlier QGC keeps its entries of version 0, decompressed, in a directory of their own
    QTest::newRow("decompressed, of an earlier QGC") << 0U << false;
    QTest::newRow("as downloaded") << 1U << true;
    QTest::newRow("of a later QGC") << 2U << false;
}

// An entry is kept as it was downloaded, possibly compressed, which its version says. An entry of another version is
// removed, as an earlier QGC removes those of this one
void ComponentInformationCacheTest::_version_test()
{
    QFETCH(uint, version);
    QFETCH(bool, kept);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const CacheTestMeta meta{.version = version};
    QVERIFY(TestFixtures::writeFile(directory.filePath(QStringLiteral("entry.meta")),
                                    QByteArray(reinterpret_cast<const char*>(&meta), sizeof(meta))));
    QVERIFY(TestFixtures::writeFile(directory.filePath(QStringLiteral("entry.cache")), "content"));

    if (!kept) {
        expectLogMessage("ComponentInformation.ComponentInformationCache", QtWarningMsg,
                         QRegularExpression("Validation failed"));
    }
    ComponentInformationCache cache(QDir(directory.path()), 10);
    if (!kept) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(cache.access(QStringLiteral("entry")).isEmpty(), !kept);
    QCOMPARE(QFile::exists(directory.filePath(QStringLiteral("entry.cache"))), kept);

    // A new entry has the version of a file as it was downloaded
    const QString downloaded = directory.filePath(QStringLiteral("downloaded.json.xz"));
    QVERIFY(TestFixtures::writeFile(downloaded, "compressed"));
    QVERIFY(!cache.insert(QStringLiteral("new"), downloaded).isEmpty());
    const QByteArray newMeta = TestFixtures::readFile(directory.filePath(QStringLiteral("new.meta")));
    CacheTestMeta written;
    QCOMPARE(newMeta.size(), static_cast<qsizetype>(sizeof(written)));
    std::memcpy(&written, newMeta.constData(), sizeof(written));
    QCOMPARE(written.version, 1U);
}

void ComponentInformationCacheTest::_incomplete_entry_test_data()
{
    QTest::addColumn<QString>("leftover");

    QTest::newRow("data without meta") << QStringLiteral("entry.cache");
    QTest::newRow("meta without data") << QStringLiteral("entry.meta");
}

// A file left without its counterpart, as a removal of a file that is still open leaves it on Windows, is not an
// entry: insert() replaces it rather than keep a file that access() never finds
void ComponentInformationCacheTest::_incomplete_entry_test()
{
    QFETCH(QString, leftover);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    ComponentInformationCache cache(QDir(directory.path()), 10);
    QVERIFY(TestFixtures::writeFile(directory.filePath(leftover), "stale"));
    QVERIFY(cache.access(QStringLiteral("entry")).isEmpty());

    const QString downloaded = directory.filePath(QStringLiteral("downloaded.json"));
    QVERIFY(TestFixtures::writeFile(downloaded, "fresh"));
    const QString cached = cache.insert(QStringLiteral("entry"), downloaded);
    QVERIFY(!cached.isEmpty());
    QCOMPARE(cache.access(QStringLiteral("entry")), cached);
    QCOMPARE(TestFixtures::readFile(cached), QByteArray("fresh"));
}

UT_REGISTER_TEST(ComponentInformationCacheTest, TestLabel::Unit, TestLabel::Vehicle)
