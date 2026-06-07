#include "QGeoFileTileCacheQGCTest.h"

#include <QtCore/QBuffer>
#include <QtGui/QImage>

#include "QGCMapTasks.h"
#include "QGCMapUrlEngine.h"
#include "QGCTileCache.h"
#include "QGeoFileTileCacheQGC.h"

class TestableQGeoFileTileCacheQGC : public QGeoFileTileCacheQGC
{
public:
    explicit TestableQGeoFileTileCacheQGC(const QVariantMap& parameters) : QGeoFileTileCacheQGC(parameters, nullptr) {}

    using QGeoFileTileCacheQGC::get;
    using QGeoFileTileCacheQGC::insert;
};

void QGeoFileTileCacheQGCTest::_testCreateFetchTileTaskValidation()
{
    const QStringList providerTypes = UrlFactory::getProviderTypes();
    QVERIFY(!providerTypes.isEmpty());
    QGCFetchTileTask* task = QGCTileCache::createFetchTileTask(providerTypes.first(), 1, 2, 3);
    QVERIFY(task != nullptr);
    delete task;
}

void QGeoFileTileCacheQGCTest::_testMemoryInsertAndGet()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    TestableQGeoFileTileCacheQGC cache(QVariantMap{});
    const QGeoTileSpec spec = tileSpec(mapId, 10, 20, 3);

    // QGeoFileTileCacheQGC overrides get()/insert() as no-ops so the base QGeoFileTileCache
    // (memory + flat-file) never shadows the SQLite store: a base-cache hit would bypass
    // expiry/ETag/must-revalidate. So even a decodable PNG inserted into the memory cache
    // is not retrievable via get() — the SQLite cache is the sole authority.
    QImage image(1, 1, QImage::Format_ARGB32);
    image.fill(Qt::red);
    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(image.save(&buffer, "PNG"));
    buffer.close();

    cache.insert(spec, pngBytes, QStringLiteral("png"), QAbstractGeoTileCache::MemoryCache);
    const QSharedPointer<QGeoTileTexture> texture = cache.get(spec);
    QVERIFY(texture.isNull());
}

void QGeoFileTileCacheQGCTest::_testDiskOnlyInsertDoesNotPopulateMemory()
{
    const int mapId = validMapId();
    QVERIFY(mapId > 0);

    TestableQGeoFileTileCacheQGC cache(QVariantMap{});
    const QGeoTileSpec spec = tileSpec(mapId, 11, 21, 3);

    cache.insert(spec, QByteArrayLiteral("img_data"), QStringLiteral("png"), QAbstractGeoTileCache::DiskCache);
    const QSharedPointer<QGeoTileTexture> texture = cache.get(spec);
    QVERIFY(texture.isNull());
}

UT_REGISTER_TEST(QGeoFileTileCacheQGCTest, TestLabel::Unit)
