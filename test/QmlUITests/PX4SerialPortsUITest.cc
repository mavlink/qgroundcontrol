#include "PX4SerialPortsUITest.h"

#include <QtCore/QPointer>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "MockLink.h"

UT_REGISTER_TEST(PX4SerialPortsUITest, TestLabel::Integration)

// MockLink runs MAVLink on TELEM1 and leaves TELEM2 unused. The page opens on In Use since a port is in use.
void PX4SerialPortsUITest::_inUseFilterHidesUnusedPorts()
{
    static constexpr int kTelem1PortId = 101;
    static constexpr int kTelem2PortId = 102;
    const QString telem1 = QStringLiteral("ports_port_%1").arg(kTelem1PortId);
    const QString telem2 = QStringLiteral("ports_port_%1").arg(kTelem2PortId);

    runWithMockLink([] { return MockLink::startPX4MockLink(); },
                    [&](QPointer<MockLink> /*mockLink*/, Vehicle* /*vehicle*/) {
                        navigateToConfigureView();
                        if (QTest::currentTestFailed()) {
                            return;
                        }
                        QVERIFY(clickSidebarButton(QStringLiteral("vehicleConfig_comp_Ports")));

                        QQuickItem* const filterBar = findVisibleItem(_rootItem, QStringLiteral("ports_filter"), 3000);
                        QVERIFY(filterBar);
                        QVERIFY(findVisibleItem(_rootItem, telem1, 3000));
                        QVERIFY(!findVisibleItem(_rootItem, telem2, 0));
                        QVERIFY(findItem(_rootItem, telem2));

                        QQuickItem* const summary = findVisibleItem(_rootItem, QStringLiteral("ports_summary"), 0);
                        QVERIFY(summary);
                        const auto [visibleRows, totalRows] = countRows(QStringLiteral("ports_port_"));
                        QVERIFY(visibleRows < totalRows);
                        QCOMPARE(summary->property("text").toString(),
                                 QStringLiteral("Showing %1 of %2 ports.").arg(visibleRows).arg(totalRows));

                        QVERIFY(QMetaObject::invokeMethod(filterBar, "activated", Q_ARG(int, 0)));
                        QTRY_VERIFY(findVisibleItem(_rootItem, telem2, 0));
                        QCOMPARE(summary->property("text").toString(),
                                 QStringLiteral("Showing all %1 ports.").arg(totalRows));

                        QVERIFY(QMetaObject::invokeMethod(filterBar, "activated", Q_ARG(int, 1)));
                        QTRY_VERIFY(!findVisibleItem(_rootItem, telem2, 0));
                        QVERIFY(findVisibleItem(_rootItem, telem1, 0));
                    });
}
