// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#include "SerialLinkTest.h"

#include "LinkManager.h"
#include "MockSerialPort.h"
#include "SerialLink.h"
#include "SerialPlatform.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

UT_REGISTER_TEST(SerialLinkTest, TestLabel::Integration, TestLabel::Comms)

namespace {

SharedLinkInterfacePtr createSerialLink(LinkManager *linkManager, const QString &portName)
{
    SharedLinkConfigurationPtr config(new SerialConfiguration(QStringLiteral("SerialLinkTest")));
    auto *serialConfig = qobject_cast<SerialConfiguration *>(config.get());
    serialConfig->setPortName(portName);

    if (!linkManager->createConnectedLink(config)) {
        return nullptr;
    }
    return linkManager->sharedLinkInterfacePointerForLink(serialConfig->link());
}

}  // namespace

void SerialLinkTest::init()
{
    CommsTest::init();
    // The open-failure case logs Comms.SerialLink warnings; behavior is asserted via communicationError.
    ignoreLogMessage("Comms.SerialLink", QtWarningMsg, QRegularExpression(QStringLiteral(".*")));
}

void SerialLinkTest::cleanup()
{
    SerialPlatform::setPortFactoryForTest({});
    CommsTest::cleanup();
}

void SerialLinkTest::_loopbackRoundTrip_deliversBytesReceived()
{
    SerialPlatform::setPortFactoryForTest([](const QString &name, QObject *parent) -> QGCSerialPort * {
        auto *port = new MockSerialPort(name, parent);
        port->setLoopback(true);
        return port;
    });

    SharedLinkInterfacePtr link = createSerialLink(linkManager(), QStringLiteral("MOCK0"));
    QVERIFY(link);
    QTRY_VERIFY_WITH_TIMEOUT(link->isConnected(), 3000);

    QSignalSpy rxSpy(link.get(), &LinkInterface::bytesReceived);
    link->writeBytesThreadSafe("hello", 5);

    QTRY_COMPARE_WITH_TIMEOUT(rxSpy.count(), 1, 3000);
    QCOMPARE(rxSpy.first().at(1).toByteArray(), QByteArrayLiteral("hello"));

    link->disconnect();
    QTRY_VERIFY_WITH_TIMEOUT(!link->isConnected(), 3000);
}

void SerialLinkTest::_openFailure_emitsCommunicationError()
{
    SerialPlatform::setPortFactoryForTest([](const QString &name, QObject *parent) -> QGCSerialPort * {
        auto *port = new MockSerialPort(name, parent);
        port->setOpenResult(false);
        return port;
    });

    expectAppMessage(QRegularExpression(QStringLiteral("mock open refused")));

    SharedLinkInterfacePtr link = createSerialLink(linkManager(), QStringLiteral("MOCK0"));
    QVERIFY(link);

    QSignalSpy errorSpy(link.get(), &LinkInterface::communicationError);
    QTRY_COMPARE_WITH_TIMEOUT(errorSpy.count(), 1, 3000);
    verifyExpectedLogMessage();
    QVERIFY(!link->isConnected());
}
