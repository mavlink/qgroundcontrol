#include "GPSCorrectionLossAlertTest.h"

#include <QtTest/QTest>

#include "GPSCorrectionLossAlert.h"

namespace {
using Event = GPSCorrectionLossAlert::Event;
using Vehicles = QList<GPSCorrectionLossAlert::Vehicle>;

constexpr qint64 HOLD_OFF = GPSCorrectionLossAlert::HOLD_OFF.count();
constexpr qint64 START = 100000;

const Vehicles RTK_VEHICLE{{.id = 1, .rtkFix = true}};
const Vehicles PLAIN_VEHICLE{{.id = 1, .rtkFix = false}};
}  // namespace

void GPSCorrectionLossAlertTest::_tellsOncePerInterruption()
{
    GPSCorrectionLossAlert alert;
    qint64 now = START;
    QCOMPARE(alert.update(true, RTK_VEHICLE, now), Event::None);

    // The stream has to stay away for the hold-off, whatever the vehicle's fix becomes meanwhile.
    QCOMPARE(alert.update(false, RTK_VEHICLE, now += 1000), Event::None);
    QCOMPARE(alert.update(false, PLAIN_VEHICLE, now += HOLD_OFF - 1), Event::None);
    QCOMPARE(alert.update(false, PLAIN_VEHICLE, now += 1), Event::Lost);
    QCOMPARE(alert.vehicles(), QList<int>{1});
    QCOMPARE(alert.update(false, PLAIN_VEHICLE, now += 10 * HOLD_OFF), Event::None);

    QCOMPARE(alert.update(true, PLAIN_VEHICLE, now += 1000), Event::Resumed);
    QCOMPARE(alert.update(true, RTK_VEHICLE, now += 1000), Event::None);

    // The next interruption is told again, after its own hold-off.
    QCOMPARE(alert.update(false, RTK_VEHICLE, now += 1000), Event::None);
    QCOMPARE(alert.update(false, RTK_VEHICLE, now += HOLD_OFF), Event::Lost);
    QCOMPARE(alert.update(false, RTK_VEHICLE, now += 1000), Event::None);
}

void GPSCorrectionLossAlertTest::_namesDependentVehicles()
{
    GPSCorrectionLossAlert alert;
    const Vehicles vehicles{{.id = 7, .rtkFix = true}, {.id = 3, .rtkFix = false}, {.id = 2, .rtkFix = true}};
    QCOMPARE(alert.update(true, vehicles, START), Event::None);
    QCOMPARE(alert.update(false, vehicles, START + 1000), Event::None);
    // Vehicle 7 went away during the hold-off; vehicle 3 never had an RTK fix from the stream.
    const Vehicles remaining{{.id = 3, .rtkFix = false}, {.id = 2, .rtkFix = false}};
    QCOMPARE(alert.update(false, remaining, START + 1000 + HOLD_OFF), Event::Lost);
    QCOMPARE(alert.vehicles(), QList<int>{2});
}

void GPSCorrectionLossAlertTest::_staysSilent_data()
{
    QTest::addColumn<Vehicles>("whileFresh");
    QTest::addColumn<Vehicles>("afterwards");
    QTest::addColumn<qint64>("interruption");

    QTest::newRow("no-vehicle") << Vehicles{} << Vehicles{} << 10 * HOLD_OFF;
    QTest::newRow("vehicle-without-rtk-fix") << PLAIN_VEHICLE << PLAIN_VEHICLE << 10 * HOLD_OFF;
    // An RTK fix the vehicle reports only once the stream is gone does not come from the stream.
    QTest::newRow("rtk-fix-after-the-stream") << PLAIN_VEHICLE << RTK_VEHICLE << 10 * HOLD_OFF;
    QTest::newRow("vehicle-gone") << RTK_VEHICLE << Vehicles{} << 10 * HOLD_OFF;
    QTest::newRow("stream-back-within-hold-off") << RTK_VEHICLE << RTK_VEHICLE << HOLD_OFF - 1;
}

void GPSCorrectionLossAlertTest::_staysSilent()
{
    QFETCH(Vehicles, whileFresh);
    QFETCH(Vehicles, afterwards);
    QFETCH(qint64, interruption);
    GPSCorrectionLossAlert alert;
    QCOMPARE(alert.update(true, whileFresh, START), Event::None);
    QCOMPARE(alert.update(false, afterwards, START + 1000), Event::None);
    QCOMPARE(alert.update(false, afterwards, START + 1000 + interruption), Event::None);
    QCOMPARE(alert.update(true, afterwards, START + 2000 + interruption), Event::None);
}

void GPSCorrectionLossAlertTest::_resumedNeedsAVehicleToTell()
{
    GPSCorrectionLossAlert alert;
    QCOMPARE(alert.update(true, RTK_VEHICLE, START), Event::None);
    QCOMPARE(alert.update(false, RTK_VEHICLE, START + 1000), Event::None);
    QCOMPARE(alert.update(false, RTK_VEHICLE, START + 1000 + HOLD_OFF), Event::Lost);
    // The vehicle disconnected before the stream came back, so its return is told to nobody, and neither is the
    // next interruption.
    QCOMPARE(alert.update(true, Vehicles{}, START + 2000 + HOLD_OFF), Event::None);
    QCOMPARE(alert.update(false, Vehicles{}, START + 3000 + HOLD_OFF), Event::None);
    QCOMPARE(alert.update(false, Vehicles{}, START + 3000 + 3 * HOLD_OFF), Event::None);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSCorrectionLossAlertTest, TestLabel::Unit)
