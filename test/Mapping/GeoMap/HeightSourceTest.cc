#include "HeightSourceTest.h"

#include <cmath>

#include "HeightField.h"
#include "HeightSource.h"

namespace {
constexpr TileMath::TileKey kKey{34, 22, 6};

/// World position of the center of a sample of the tile that a procedural source synthesizes for kKey; row 0 is
/// the north edge. The field answers there with the sample itself.
QPointF sampleCenter(int row, int col)
{
    const QPointF minCorner = TileMath::tileMinCorner(kKey);
    const double span = TileMath::tileSpanAtZoom(kKey.zoom);
    const double cell = span / ProceduralHeightSource::kSynthGridSize;
    return QPointF(minCorner.x() + ((col + 0.5) * cell), minCorner.y() + span - ((row + 0.5) * cell));
}

double debugHeightAt(const QPointF& world)
{
    const double phase = (2.0 * M_PI) / DebugHeightSource::kWavelength;
    return ((std::sin(world.x() * phase) * std::cos(world.y() * phase)) + 1.0) / 2.0 * DebugHeightSource::kAmplitude;
}
}  // namespace

void HeightSourceTest::_debugTileDeliveredThroughEventLoop()
{
    HeightField field;
    DebugHeightSource source;
    source.setHeightField(&field);

    QVERIFY(source.requestTile(kKey));
    QCOMPARE(field.tileCount(), 0);  // delivery is async, never re-entrant
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(kKey), TestTimeout::shortMs());
}

void HeightSourceTest::_debugTileRowMajorFromNorthWest()
{
    HeightField field;
    DebugHeightSource source;
    source.setHeightField(&field);
    QVERIFY(source.requestTile(kKey));
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(kKey), TestTimeout::shortMs());

    // The corners that a tile with swapped or mirrored rows and columns would get wrong
    const int last = ProceduralHeightSource::kSynthGridSize - 1;
    for (const QPointF& center : {sampleCenter(0, 0), sampleCenter(0, last), sampleCenter(last, 0)}) {
        QCOMPARE_LT(qAbs(field.heightAt(center) - debugHeightAt(center)), 0.01);
    }
}

void HeightSourceTest::_debugHillsWithinAmplitude()
{
    HeightField field;
    DebugHeightSource source;
    source.setHeightField(&field);
    QVERIFY(source.requestTile(kKey));
    QTRY_VERIFY_WITH_TIMEOUT(field.hasTile(kKey), TestTimeout::shortMs());

    double minHeight = DebugHeightSource::kAmplitude;
    double maxHeight = 0.0;
    for (int row = 0; row < ProceduralHeightSource::kSynthGridSize; row++) {
        for (int col = 0; col < ProceduralHeightSource::kSynthGridSize; col++) {
            const double height = field.heightAt(sampleCenter(row, col));
            QCOMPARE_GE(height, 0.0);
            QCOMPARE_LE(height, DebugHeightSource::kAmplitude);
            minHeight = qMin(minHeight, height);
            maxHeight = qMax(maxHeight, height);
        }
    }
    // Hills must actually displace the mesh
    QCOMPARE_GT(maxHeight - minHeight, 1.0);
}

void HeightSourceTest::_flatSourceGivesNoTile()
{
    HeightField field;
    FlatHeightSource source;
    source.setHeightField(&field);

    // The empty field answers 0 m already
    QVERIFY(!source.requestTile(kKey));
}

void HeightSourceTest::_tileRequestRejected()
{
    DebugHeightSource source;
    QVERIFY(!source.requestTile(kKey));  // no field to put the tile into

    HeightField field;
    source.setHeightField(&field);
    QVERIFY(!source.requestTile(TileMath::TileKey{0, 0, -1}));
}

UT_REGISTER_TEST_LIGHTWEIGHT(HeightSourceTest, TestLabel::Unit)
