#include "QGCNetworkAvailabilityMonitorTest.h"

#include <QtTest/QSignalSpy>

#include "QGCNetworkAvailabilityMonitor.h"

namespace {

/// Reports availability itself, as a platform backend would.
class ScriptedMonitor : public QGCNetworkAvailabilityMonitor
{
public:
    explicit ScriptedMonitor(bool available)
        : QGCNetworkAvailabilityMonitor(available, nullptr)
    {}

    void report(bool available) { _setAvailable(available); }
};

}  // namespace

void QGCNetworkAvailabilityMonitorTest::_notifiesChangesOnly_data()
{
    QTest::addColumn<bool>("initial");
    QTest::newRow("starts-available") << true;
    QTest::newRow("starts-unavailable") << false;
}

void QGCNetworkAvailabilityMonitorTest::_notifiesChangesOnly()
{
    QFETCH(bool, initial);
    ScriptedMonitor monitor(initial);
    QSignalSpy changes(&monitor, &QGCNetworkAvailabilityMonitor::availableChanged);
    QCOMPARE(monitor.available(), initial);

    monitor.report(initial);
    QVERIFY(changes.isEmpty());

    monitor.report(!initial);
    QCOMPARE(monitor.available(), !initial);
    QCOMPARE(changes.size(), 1);
    QCOMPARE(changes.first().first().toBool(), !initial);

    monitor.report(!initial);
    QCOMPARE(changes.size(), 1);
    monitor.report(initial);
    QCOMPARE(changes.size(), 2);
    QCOMPARE(changes.last().first().toBool(), initial);
}

UT_REGISTER_TEST(QGCNetworkAvailabilityMonitorTest, TestLabel::Unit, TestLabel::Utilities)
