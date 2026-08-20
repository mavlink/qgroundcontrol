#include "HeightSourceTest.h"

#include <QtTest/QSignalSpy>

#include <cmath>

#include "HeightSource.h"

namespace {
constexpr TileMath::TileKey kKey{34, 22, 6};
constexpr int kGridSize = 4;
constexpr int kExpectedCount = (kGridSize + 1) * (kGridSize + 1);
}  // namespace

void HeightSourceTest::_flatDeliversZeros()
{
    FlatHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);

    const int requestId = source.requestPatchHeights(kKey, kGridSize);
    QCOMPARE_GT(requestId, 0);
    QVERIFY(readySpy.isEmpty());  // delivery is async, never re-entrant

    QVERIFY(readySpy.wait());
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(readySpy.first().at(0).toInt(), requestId);

    const auto heights = readySpy.first().at(1).value<QList<float>>();
    QCOMPARE(heights.count(), kExpectedCount);
    for (float h : heights) {
        QCOMPARE(h, 0.0f);
    }
}

void HeightSourceTest::_requestIdsIncrement()
{
    FlatHeightSource source;
    const int first = source.requestPatchHeights(kKey, kGridSize);
    const int second = source.requestPatchHeights(kKey, kGridSize);
    QCOMPARE_GT(second, first);
}

void HeightSourceTest::_cancelPreventsDelivery()
{
    FlatHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    const int cancelledId = source.requestPatchHeights(kKey, kGridSize);
    source.cancelRequest(cancelledId);
    const int liveId = source.requestPatchHeights(kKey, kGridSize);

    // Queued delivery preserves order: once liveId arrives, cancelledId would already
    // have been delivered if cancellation had not removed it.
    QVERIFY(readySpy.wait());
    QCOMPARE(readySpy.count(), 1);
    QCOMPARE(readySpy.first().at(0).toInt(), liveId);
    QVERIFY(failedSpy.isEmpty());
}

void HeightSourceTest::_invalidGridSizeFails()
{
    FlatHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);
    QSignalSpy failedSpy(&source, &HeightSource::patchHeightsFailed);

    const int requestId = source.requestPatchHeights(kKey, 0);
    QVERIFY(failedSpy.wait());
    QCOMPARE(failedSpy.first().at(0).toInt(), requestId);
    QVERIFY(readySpy.isEmpty());
}

void HeightSourceTest::_debugDeterministic()
{
    DebugHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);

    source.requestPatchHeights(kKey, kGridSize);
    source.requestPatchHeights(kKey, kGridSize);
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.count(), 2, 1000);

    const auto first = readySpy.at(0).at(1).value<QList<float>>();
    const auto second = readySpy.at(1).at(1).value<QList<float>>();
    QCOMPARE(first, second);
    QCOMPARE(first.count(), kExpectedCount);

    float minHeight = first.first();
    float maxHeight = first.first();
    for (float h : first) {
        minHeight = qMin(minHeight, h);
        maxHeight = qMax(maxHeight, h);
        QCOMPARE_GE(h, 0.0f);
        QCOMPARE_LE(h, static_cast<float>(DebugHeightSource::kAmplitude));
    }
    // Hills must actually displace the mesh
    QCOMPARE_GT(maxHeight - minHeight, 1.0f);
}

void HeightSourceTest::_debugRowMajorFromNorthWest()
{
    DebugHeightSource source;
    QSignalSpy readySpy(&source, &HeightSource::patchHeightsReady);

    source.requestPatchHeights(kKey, kGridSize);
    QVERIFY(readySpy.wait());
    const auto heights = readySpy.first().at(1).value<QList<float>>();

    // Recompute the expected value at the north-west corner (first entry)
    const QPointF minCorner = TileMath::tileMinCorner(kKey);
    const double span = TileMath::tileSpanAtZoom(kKey.zoom);
    const double phase = (2.0 * M_PI) / DebugHeightSource::kWavelength;
    const double nwX = minCorner.x();
    const double nwY = minCorner.y() + span;
    const float expected = static_cast<float>(((std::sin(nwX * phase) * std::cos(nwY * phase)) + 1.0) / 2.0 *
                                              DebugHeightSource::kAmplitude);

    QCOMPARE(heights.first(), expected);
}

UT_REGISTER_TEST_LIGHTWEIGHT(HeightSourceTest, TestLabel::Unit)
