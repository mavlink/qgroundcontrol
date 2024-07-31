/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SigningTest.h"
#include "MAVLinkSigning.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "MultiVehicleManager.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

SigningTest::SigningTest()
    : _multiVehicleMgr(qgcApp()->toolbox()->multiVehicleManager())
{

}

void SigningTest::_testInitSigning()
{
    const mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);

    QVERIFY(!MAVLinkSigning::initSigning(MAVLINK_COMM_0, "secret_key", nullptr));

    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, QByteArrayView(), nullptr));
    QVERIFY(!status->signing);

    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "secret_key", &MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));
    QVERIFY(status->signing);
    QVERIFY(status->signing->link_id == MAVLINK_COMM_0);
    QVERIFY(status->signing->flags & MAVLINK_SIGNING_FLAG_SIGN_OUTGOING);
    QVERIFY(status->signing->accept_unsigned_callback == &MAVLinkSigning::insecureConnectionAccceptUnsignedCallback);
    QVERIFY(memcmp(status->signing->secret_key, QCryptographicHash::hash("secret_key", QCryptographicHash::Sha256), sizeof(status->signing->secret_key)) == 0);

    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, QByteArrayView("1", 32), &MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));
    QVERIFY(memcmp(status->signing->secret_key, QCryptographicHash::hash(QByteArrayView("1", 32), QCryptographicHash::Sha256), sizeof(status->signing->secret_key)) == 0);
}

void SigningTest::_testCheckSigningLinkId()
{
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "secret_key", &MAVLinkSigning::insecureConnectionAccceptUnsignedCallback));
    const mavlink_heartbeat_t heartbeat = {0};
    mavlink_message_t message;
    (void) mavlink_msg_heartbeat_encode_chan(1, MAV_COMP_ID_USER1, MAVLINK_COMM_0, &message, &heartbeat);
    QVERIFY(MAVLinkSigning::checkSigningLinkId(MAVLINK_COMM_0, message));
}

void SigningTest::_testCreateSetupSigning()
{
    QVERIFY(MAVLinkSigning::initSigning(MAVLINK_COMM_0, "secret_key", &MAVLinkSigning::secureConnectionAccceptUnsignedCallback));
    const mavlink_system_t target_system = {1, MAV_COMP_ID_AUTOPILOT1};
    mavlink_setup_signing_t setup_signing;
    MAVLinkSigning::createSetupSigning(MAVLINK_COMM_0, target_system, setup_signing);
    QVERIFY(setup_signing.initial_timestamp != 0);
    QCOMPARE(setup_signing.target_system, target_system.sysid);
    QCOMPARE(setup_signing.target_component, target_system.compid);
}

void SigningTest::_testDenyUnsignedMessages()
{
    QSignalSpy spyParamsReady(_multiVehicleMgr, SIGNAL(parameterReadyVehicleAvailableChanged(bool)));

    // Start a MockLink with signing enabled for incoming traffic. But QGC side of pipe is not setup for signing.
    Q_ASSERT(!_mockLink);
    _mockLink = MockLink::startMockLink(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, false /* sendStatusText */, false /* isSecureConnection */, "SigningKey");
    QVERIFY(_mockLink);

    // Signing should not be enabled for QGC outgoing traffic
    const mavlink_status_t *status = mavlink_get_channel_status(_mockLink->mavlinkChannel());
    const mavlink_signing_t *signing = status->signing;
    QVERIFY(!signing);

    // Parameters should not download
    QCOMPARE(spyParamsReady.wait(60000), true);
    auto arguments = spyParamsReady.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), false);
}

void SigningTest::_testBadSignature()
{
    QSignalSpy spyParamsReady(_multiVehicleMgr, SIGNAL(parameterReadyVehicleAvailableChanged(bool)));

    // Setup QGC with a signing key which is different than the vehicle signing key
    auto appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
    appSettings->mavlink2SigningKey()->setRawValue("QGCSigningKey");

    // Start a Vehicle with signing enabled for incoming traffic.
    _mockLink = MockLink::startMockLink(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, false /* sendStatusText */, false /* isSecureConnection */, "MockLinkSigningKey");
    QVERIFY(_mockLink);
    QSignalSpy spyLastSigningStatusChanged(_mockLink, SIGNAL(lastSigningStatusChanged(int)));

    // Signing should be enabled on outgoing QGC side
    const mavlink_status_t *status = mavlink_get_channel_status(_mockLink->mavlinkChannel());
    const mavlink_signing_t *signing = status->signing;
    QVERIFY(signing);

    // Parameters should not download
    QCOMPARE(spyParamsReady.wait(60000), true);
    auto arguments = spyParamsReady.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toBool(), false);

    // Put signing key back to default
    appSettings->mavlink2SigningKey()->setRawValue("");
}

void SigningTest::_testGoodSignatures()
{
    QString signingKey("SigningKey");

    QSignalSpy spyActiveVehicleChanged(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged);

    // Setup QGC with the same signing key as the vehicle
    auto appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
    appSettings->mavlink2SigningKey()->setRawValue(signingKey);

    // Start a Vehicle with signing enabled for incoming traffic.
    _mockLink = MockLink::startMockLink(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, false /* sendStatusText */, false /* isSecureConnection */, signingKey);
    QVERIFY(_mockLink);

    // Signing should be enabled on outgoing QGC side
    const mavlink_status_t *status = mavlink_get_channel_status(_mockLink->mavlinkChannel());
    const mavlink_signing_t *signing = status->signing;
    QVERIFY(signing);

    // Wait for the Vehicle to get created
    QCOMPARE(spyActiveVehicleChanged.wait(10000), true);
    _vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    QVERIFY(_vehicle);

    // Put signing key back to default
    appSettings->mavlink2SigningKey()->setRawValue("");
}

void SigningTest::_testNoSigning()
{
    // Setup QGC with no signing key
    auto appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
    appSettings->mavlink2SigningKey()->setRawValue("");

    // Start a Vehicle with no signing
    _mockLink = MockLink::startMockLink(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, false /* sendStatusText */, false /* isSecureConnection */, "");
    QVERIFY(_mockLink);

    const mavlink_status_t *status = mavlink_get_channel_status(_mockLink->mavlinkChannel());
    const mavlink_signing_t *signing = status->signing;
    QVERIFY(!signing);

    QSignalSpy spyMockLink(_mockLink, SIGNAL(lastSigningStatusChanged(int)));

    // Vehicle should still be created
    auto multiVehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    QSignalSpy spyVehicle(multiVehicleMgr, &MultiVehicleManager::activeVehicleChanged);
    QCOMPARE(spyVehicle.wait(5000), true);
    QCOMPARE(spyVehicle.count(), 1);

    // MockLink shouldn't report signing status changes, since no signing is enabled
    QCOMPARE(spyMockLink.count(), 0);
}
