#include "SensorCalibrationSideTest.h"
#include "SensorCalibrationSide.h"

#include <QtTest/QTest>

void SensorCalibrationSideTest::_testSideToMask()
{
    QCOMPARE(SensorCalibrationSide::sideToMask(CalibrationSide::Down),
             static_cast<int>(CalibrationSideMask::MaskDown));
    QCOMPARE(SensorCalibrationSide::sideToMask(CalibrationSide::Up),
             static_cast<int>(CalibrationSideMask::MaskUp));
    QCOMPARE(SensorCalibrationSide::sideToMask(CalibrationSide::Left),
             static_cast<int>(CalibrationSideMask::MaskLeft));
    QCOMPARE(SensorCalibrationSide::sideToMask(CalibrationSide::Right),
             static_cast<int>(CalibrationSideMask::MaskRight));
    QCOMPARE(SensorCalibrationSide::sideToMask(CalibrationSide::Front),
             static_cast<int>(CalibrationSideMask::MaskFront));
    QCOMPARE(SensorCalibrationSide::sideToMask(CalibrationSide::Back),
             static_cast<int>(CalibrationSideMask::MaskBack));
}

void SensorCalibrationSideTest::_testMaskToSide()
{
    QCOMPARE(SensorCalibrationSide::maskToSide(CalibrationSideMask::MaskDown), CalibrationSide::Down);
    QCOMPARE(SensorCalibrationSide::maskToSide(CalibrationSideMask::MaskUp), CalibrationSide::Up);
    QCOMPARE(SensorCalibrationSide::maskToSide(CalibrationSideMask::MaskLeft), CalibrationSide::Left);
    QCOMPARE(SensorCalibrationSide::maskToSide(CalibrationSideMask::MaskRight), CalibrationSide::Right);
    QCOMPARE(SensorCalibrationSide::maskToSide(CalibrationSideMask::MaskFront), CalibrationSide::Front);
    QCOMPARE(SensorCalibrationSide::maskToSide(CalibrationSideMask::MaskBack), CalibrationSide::Back);

    // Test with combined mask - should return first set bit (Down has highest priority)
    QCOMPARE(SensorCalibrationSide::maskToSide(CalibrationSideMask::MaskDown | CalibrationSideMask::MaskUp),
             CalibrationSide::Down);
}

void SensorCalibrationSideTest::_testParseSideText()
{
    QCOMPARE(SensorCalibrationSide::parseSideText("down"), CalibrationSide::Down);
    QCOMPARE(SensorCalibrationSide::parseSideText("up"), CalibrationSide::Up);
    QCOMPARE(SensorCalibrationSide::parseSideText("left"), CalibrationSide::Left);
    QCOMPARE(SensorCalibrationSide::parseSideText("right"), CalibrationSide::Right);
    QCOMPARE(SensorCalibrationSide::parseSideText("front"), CalibrationSide::Front);
    QCOMPARE(SensorCalibrationSide::parseSideText("back"), CalibrationSide::Back);

    // Unknown text should return Down as default
    QCOMPARE(SensorCalibrationSide::parseSideText("unknown"), CalibrationSide::Down);
    QCOMPARE(SensorCalibrationSide::parseSideText(""), CalibrationSide::Down);
}

void SensorCalibrationSideTest::_testSideName()
{
    QCOMPARE(SensorCalibrationSide::sideName(CalibrationSide::Down), QStringLiteral("Down"));
    QCOMPARE(SensorCalibrationSide::sideName(CalibrationSide::Up), QStringLiteral("Up"));
    QCOMPARE(SensorCalibrationSide::sideName(CalibrationSide::Left), QStringLiteral("Left"));
    QCOMPARE(SensorCalibrationSide::sideName(CalibrationSide::Right), QStringLiteral("Right"));
    QCOMPARE(SensorCalibrationSide::sideName(CalibrationSide::Front), QStringLiteral("Front"));
    QCOMPARE(SensorCalibrationSide::sideName(CalibrationSide::Back), QStringLiteral("Back"));
}
