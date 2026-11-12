#include "GPSPipeTransportTest.h"

#include "Transport/Support/PipeTransportChecks.h"

void GPSPipeTransportTest::_checks_data()
{
    QTest::addColumn<int>("check");
    int index = 0;
    for (const auto& check : GPSTest::pipeTransportChecks()) {
        QTest::newRow(check.name) << index++;
    }
}

void GPSPipeTransportTest::_checks()
{
    QFETCH(int, check);
    const QString failure = GPSTest::pipeTransportChecks()[static_cast<size_t>(check)].run();
    QVERIFY2(failure.isEmpty(), qPrintable(failure));
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSPipeTransportTest, TestLabel::Unit)
