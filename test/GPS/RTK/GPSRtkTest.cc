#include "GPSRtkTest.h"

#include <limits>
#include <utility>

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>
#include <QtTest/QSignalSpy>

#include "AutoConnectSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSManager.h"
#include "GPSPositionService.h"
#include "GPSRTKFactGroup.h"
#include "GPSRtk.h"
#include "GpsTestHelpers.h"
#include "LogManager.h"
#include "QGCLoggingCategoryManager.h"
#include "QGroundControlQmlGlobal.h"
#include "RTCMMavlink.h"
#include "RTKConnectionPolicy.h"
#include "RTKSettings.h"
#include "ScriptedGPSTransport.h"
#include "SettingsManager.h"
#include "TCPGPSTransport.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

namespace {
GPSPositionReport fixReport(GPSFixQuality fixType)
{
    GPSPositionReport report;
    report.navigation.fixType = fixType;
    return report;
}
}  // namespace

void GPSRtkTest::_currentBaseSaveValidity_data()
{
    QTest::addColumn<QString>("field");
    QTest::addColumn<double>("value");
    QTest::addColumn<bool>("expected");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    QTest::newRow("known-zero-accuracy") << "currentAccuracy" << 0.0 << true;
    QTest::newRow("known-accuracy") << "currentAccuracy" << 1.25 << true;
    QTest::newRow("unavailable-accuracy") << "currentAccuracy" << nan << false;
    QTest::newRow("infinite-accuracy") << "currentAccuracy" << infinity << false;
    QTest::newRow("negative-accuracy") << "currentAccuracy" << -1.0 << false;
    QTest::newRow("wire-accuracy-overflow") << "currentAccuracy" << 429496.75 << false;
    QTest::newRow("float-accuracy-overflow") << "currentAccuracy" << 1e100 << false;
    QTest::newRow("unavailable-latitude") << "currentLatitude" << nan << false;
    QTest::newRow("invalid-latitude") << "currentLatitude" << 91.0 << false;
    QTest::newRow("unavailable-longitude") << "currentLongitude" << nan << false;
    QTest::newRow("invalid-longitude") << "currentLongitude" << -181.0 << false;
    QTest::newRow("unavailable-ellipsoid-altitude") << "currentAltitude" << nan << false;
    QTest::newRow("infinite-ellipsoid-altitude") << "currentAltitude" << infinity << false;
    QTest::newRow("wire-altitude-overflow") << "currentAltitude" << 21474838.0 << false;
}

void GPSRtkTest::_currentBaseSaveValidity()
{
    QFETCH(QString, field);
    QFETCH(double, value);
    QFETCH(bool, expected);
    GPSRTKFactGroup facts;
    facts.currentLatitude()->setRawValue(47.0);
    facts.currentLongitude()->setRawValue(8.0);
    facts.currentAltitude()->setRawValue(500.0f);
    facts.currentAccuracy()->setRawValue(1.0);
    QVERIFY(!facts.canSaveCurrentBasePosition());
    facts.valid()->setRawValue(true);
    QVERIFY(facts.canSaveCurrentBasePosition());
    QSignalSpy changes(&facts, &GPSRTKFactGroup::currentBasePositionChanged);
    Fact* const changed = facts.property(field.toUtf8().constData()).value<Fact*>();
    QVERIFY(changed);
    changed->setRawValue(value);
    QVERIFY(!changes.isEmpty());
    QCOMPARE(facts.canSaveCurrentBasePosition(), expected);
    QCOMPARE(facts.property("canSaveCurrentBasePosition").toBool(), expected);
    facts.valid()->setRawValue(false);
    QVERIFY(!facts.canSaveCurrentBasePosition());
}

void GPSRtkTest::_snapshotUsageEvidence_data()
{
    QTest::addColumn<int>("inViewValue");
    QTest::addColumn<int>("usedValue");
    QTest::addColumn<int>("expectedInView");
    QTest::addColumn<int>("expectedUsage");
    QTest::newRow("unavailable") << -1 << -1 << -1 << -1;
    QTest::newRow("count-only") << -1 << 7 << -1 << 7;
    QTest::newRow("unknown-usage") << 3 << -1 << 3 << -1;
    QTest::newRow("known-zero") << 3 << 0 << 3 << 0;
    QTest::newRow("known-used") << 3 << 2 << 3 << 2;
    QTest::newRow("empty") << 0 << 0 << 0 << 0;
}

void GPSRtkTest::_snapshotUsageEvidence()
{
    QFETCH(int, inViewValue);
    QFETCH(int, usedValue);
    QFETCH(int, expectedInView);
    QFETCH(int, expectedUsage);
    GPSSatelliteReport snapshot;
    snapshot.timestampUs = 1;
    if (inViewValue >= 0) {
        snapshot.inView = inViewValue;
    }
    if (usedValue >= 0) {
        snapshot.used = usedValue;
    }

    GPSRtk receiver;
    auto* facts = qobject_cast<GPSRTKFactGroup*>(receiver.gpsRtkFactGroup());
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), -1);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), -1);
    receiver._satelliteInfoUpdate(snapshot);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), expectedInView);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), expectedUsage);

    receiver.disconnectGPS();
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), -1);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), -1);
}

void GPSRtkTest::_logsFixTransitionsWithoutCoordinates()
{
    GPSRtk receiver;
    const QString category = QStringLiteral("GPS.RTK.GPSRtk");
    auto* logging = QGCLoggingCategoryManager::instance();
    const bool wasEnabled = logging->isCategoryEnabled(category);
    if (!wasEnabled) {
        logging->setCategoryEnabled(category, true);
    }
    const auto restore = qScopeGuard([logging, category, wasEnabled] {
        if (!wasEnabled) {
            logging->setCategoryEnabled(category, false);
        }
    });
    const auto initialCount = LogManager::capturedMessages(category).size();
    expectLogMessage("GPS.RTK.GPSRtk", QtDebugMsg, QRegularExpression(QStringLiteral("Receiver fix changed:")));
    receiver._positionUpdate(fixReport(GPSFixQuality::Fix3D));
    verifyExpectedLogMessage();
    QCOMPARE(LogManager::capturedMessages(category).size(), initialCount + 1);
    expectLogMessage("GPS.RTK.GPSRtk", QtDebugMsg, QRegularExpression(QStringLiteral("Receiver fix changed: 1")));
    receiver._positionUpdate(fixReport(GPSFixQuality::NoFix));
    verifyExpectedLogMessage();
    QCOMPARE(LogManager::capturedMessages(category).size(), initialCount + 2);
    receiver.disconnectGPS();
    expectLogMessage("GPS.RTK.GPSRtk", QtDebugMsg, QRegularExpression(QStringLiteral("Receiver fix changed: 1")));
    receiver._positionUpdate(fixReport(GPSFixQuality::NoFix));
    verifyExpectedLogMessage();
    QCOMPARE(LogManager::capturedMessages(category).size(), initialCount + 3);
    for (const auto& entry : LogManager::capturedMessages(category)) {
        QVERIFY(QRegularExpression(QStringLiteral("^Receiver fix changed: [0-9]+$")).match(entry.message).hasMatch());
    }
}

UT_REGISTER_TEST(GPSRtkTest, TestLabel::Unit)

void GPSRtkTest::_testCoreAvailableWithoutReceiver()
{
    GPSRtk rtk;
    QVERIFY(!rtk.connected());
    auto* facts = qobject_cast<GPSRTKFactGroup*>(rtk.gpsRtkFactGroup());
    QVERIFY(facts);
    QVERIFY(!facts->connected()->rawValue().toBool());
    QVERIFY(QFile::exists(QStringLiteral(":/json/Vehicle/GPSRTKFact.json")));
    QCOMPARE(rtk.property("facts").value<GPSRTKFactGroup*>(), facts);
    QCOMPARE(QGroundControlQmlGlobal::staticMetaObject.indexOfProperty("gpsRtk"), -1);
}

namespace {
struct BlockedOpen
{
    QSemaphore entered;
    QSemaphore release;
    std::atomic_bool sawCancellation = false;
};

GPSProvider::TransportFactory blockedFactory(const std::shared_ptr<BlockedOpen>& gate)
{
    return [gate](const std::atomic_bool& stop) {
        gate->entered.release();
        gate->release.acquire();
        gate->sawCancellation = stop.load();
        return std::unique_ptr<GPSTransport>{};
    };
}
}  // namespace

void GPSRtkTest::_notificationsFollowCompletedConnection_data()
{
    QTest::addColumn<QString>("phase");
    QTest::addColumn<QString>("action");
    for (const QString& phase :
         {QStringLiteral("manufacturer"), QStringLiteral("error-message"), QStringLiteral("receiver")}) {
        for (const QString& action :
             {QStringLiteral("replace"), QStringLiteral("disconnect"), QStringLiteral("delete")}) {
            QTest::newRow(qPrintable(phase + '-' + action)) << phase << action;
        }
    }
}

void GPSRtkTest::_notificationsFollowCompletedConnection()
{
    QFETCH(QString, phase);
    QFETCH(QString, action);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), 6);
    saved.setFactValue(settings->useFixedBasePosition(), 0);
    auto firstGate = std::make_shared<BlockedOpen>();
    auto replacementGate = std::make_shared<BlockedOpen>();
    auto receiver = std::make_unique<GPSRtk>();
    receiver->_setError(GPSConnectionError::OpenFailed, QStringLiteral("previous failure"));
    QPointer<GPSProvider> first;
    QPointer<GPSProvider> replacement;
    bool handled = false;
    QObject observer;
    const auto cleanup = qScopeGuard([&] {
        for (const auto& provider : {first, replacement}) {
            if (provider) {
                provider->stop();
            }
        }
        if (receiver && receiver->_session.provider) {
            receiver->_session.provider->stop();
        }
        firstGate->release.release();
        replacementGate->release.release();
        if (receiver) {
            receiver->disconnectGPS();
        }
        for (const auto& provider : {first, replacement}) {
            if (provider) {
                QVERIFY(provider->wait(TestTimeout::mediumMs()));
            }
        }
    });
    const auto supersede = [&] {
        if (std::exchange(handled, true)) {
            return;
        }
        // Notifications arrive after the connection is installed.
        QVERIFY(receiver->hasReceiver());
        QCOMPARE(receiver->activeManufacturer(), GPSRtk::manufacturerForType(GPSType::ublox));
        QVERIFY(receiver->errorMessage().isEmpty());
        first = receiver->_session.provider;
        QVERIFY(firstGate->entered.tryAcquire(1, TestTimeout::mediumMs()));
        if (action == QStringLiteral("delete")) {
            receiver.reset();
        } else if (action == QStringLiteral("disconnect")) {
            receiver->disconnectGPS();
        } else {
            QVERIFY(receiver->connectReceiver(GPSType::passive, blockedFactory(replacementGate),
                                              QStringLiteral("replacement"), 115200));
            replacement = receiver->_session.provider;
        }
        if (first) {
            QVERIFY(!first->parent());
        }
    };
    if (phase == QStringLiteral("manufacturer")) {
        connect(settings->baseReceiverManufacturers(), &Fact::rawValueChanged, &observer, supersede);
    } else if (phase == QStringLiteral("error-message")) {
        connect(receiver.get(), &GPSRtk::errorMessageChanged, &observer, supersede);
    } else {
        connect(receiver.get(), &GPSRtk::receiverChanged, &observer, supersede);
    }
    QVERIFY(receiver->connectReceiver(GPSType::ublox, blockedFactory(firstGate)));
    QVERIFY(handled);
    if (action == QStringLiteral("delete")) {
        QVERIFY(!receiver);
    } else if (action == QStringLiteral("replace")) {
        QVERIFY(receiver->hasReceiver());
        QCOMPARE(receiver->_session.provider, replacement);
        QCOMPARE(receiver->activeManufacturer(), 7);
        QVERIFY(!receiver->_session.provider->_config.allowPersistentChanges);
        QCOMPARE(receiver->findChildren<GPSProvider*>().size(), 1);
    } else {
        QVERIFY(!receiver->hasReceiver());
        QCOMPARE(receiver->findChildren<GPSProvider*>().size(), 0);
    }
}

void GPSRtkTest::_factNotificationRetiresSession_data()
{
    QTest::addColumn<QString>("report");
    QTest::addColumn<QString>("action");
    for (const QString& report : {QStringLiteral("survey"), QStringLiteral("satellites"), QStringLiteral("ready"),
                                  QStringLiteral("disconnect"), QStringLiteral("fix")}) {
        for (const QString& action :
             {QStringLiteral("replace"), QStringLiteral("disconnect"), QStringLiteral("delete")}) {
            QTest::newRow(qPrintable(report + '-' + action)) << report << action;
        }
    }
}

void GPSRtkTest::_factNotificationRetiresSession()
{
    QFETCH(QString, report);
    QFETCH(QString, action);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), 4);
    saved.setFactValue(settings->useFixedBasePosition(), 0);
    auto gate = std::make_shared<BlockedOpen>();
    auto replacementGate = std::make_shared<BlockedOpen>();
    auto receiver = std::make_unique<GPSRtk>();
    QPointer<GPSProvider> replacement;
    bool handled = false;
    QObject observer;
    QVERIFY(receiver->connectReceiver(GPSType::ublox, blockedFactory(gate)));
    const QPointer<GPSProvider> first = receiver->_session.provider;
    const auto cleanup = qScopeGuard([&] {
        for (const auto& provider : {first, replacement}) {
            if (provider) {
                provider->stop();
            }
        }
        gate->release.release();
        replacementGate->release.release();
        if (receiver) {
            receiver->disconnectGPS();
        }
        for (const auto& provider : {first, replacement}) {
            if (provider) {
                QVERIFY(provider->wait(TestTimeout::mediumMs()));
            }
        }
    });
    QVERIFY(gate->entered.tryAcquire(1, TestTimeout::mediumMs()));
    auto* facts = receiver->gpsRtkFactGroup();
    if (report == QStringLiteral("disconnect")) {
        receiver->_onGPSConnect();
    }
    Fact* trigger = report == QStringLiteral("survey")       ? facts->currentDuration()
                    : report == QStringLiteral("satellites") ? facts->numSatellites()
                    : report == QStringLiteral("fix")        ? facts->fixType()
                                                             : facts->connected();
    connect(trigger, &Fact::rawValueChanged, &observer, [&] {
        if (std::exchange(handled, true)) {
            return;
        }
        if (action == QStringLiteral("delete")) {
            receiver.reset();
        } else if (action == QStringLiteral("disconnect")) {
            receiver->disconnectGPS();
        } else {
            QVERIFY(receiver->connectReceiver(GPSType::passive, blockedFactory(replacementGate), {}, 115200));
            replacement = receiver->_session.provider;
        }
    });
    if (report == QStringLiteral("survey")) {
        GPSSurveyReport survey;
        survey.duration = std::chrono::seconds(123);
        survey.valid = true;
        survey.active = true;
        survey.position = {.latitudeDegrees = 47, .longitudeDegrees = 8};
        survey.meanAccuracyMeters = 1.5;
        receiver->_onGPSSurveyReport(survey);
    } else if (report == QStringLiteral("satellites")) {
        GPSSatelliteReport satellites;
        satellites.timestampUs = 1;
        satellites.inView = 1;
        satellites.used = 1;
        receiver->_satelliteInfoUpdate(satellites);
    } else if (report == QStringLiteral("ready")) {
        receiver->_onGPSConnect();
    } else if (report == QStringLiteral("disconnect")) {
        receiver->disconnectGPS();
    } else {
        receiver->_positionUpdate(fixReport(GPSFixQuality::Fix3D));
    }
    QVERIFY(handled);
    QVERIFY(first && !first->parent());
    if (receiver) {
        QCOMPARE(receiver->hasReceiver(), action == QStringLiteral("replace"));
        QVERIFY(!receiver->connected());
        QVERIFY(!receiver->gpsRtkFactGroup()->valid()->rawValue().toBool());
        QCOMPARE(receiver->gpsRtkFactGroup()->numSatellitesUsed()->rawValue().toInt(), -1);
        if (replacement) {
            QVERIFY(receiver->errorMessage().isEmpty());
            QCOMPARE(receiver->activeManufacturer(), 7);
        }
    }
}

void GPSRtkTest::_settingNotificationFollowsConnection_data()
{
    QTest::addColumn<bool>("destroy");
    QTest::newRow("stop") << false;
    QTest::newRow("delete") << true;
}

void GPSRtkTest::_settingNotificationFollowsConnection()
{
#ifdef QGC_NO_SERIAL_LINK
    QSKIP("Manual serial connection requires serial support");
#else
    QFETCH(bool, destroy);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    auto* enabled = SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS();
    saved.setFactValue(settings->baseReceiverManufacturers(), 7);
    saved.setFactValue(settings->serialDevice(), QStringLiteral("/test/reentrant"));
    saved.setFactValue(settings->serialBaudRate(), 115200);
    saved.setFactValue(enabled, true);
    SerialPortManager ports(nullptr, [] {
        return QList<SerialPortManager::Port>{
            {QStringLiteral("/test/reentrant"), QStringLiteral("reentrant"), QGCSerialPortInfo::BoardTypeUnknown, {}}};
    });
    auto receiver = std::make_unique<GPSRtk>();
    receiver->setSerialPortManager(&ports);
    receiver->_serialTransportFactory = [](const QString&, const std::atomic_bool& stop) {
        while (!stop.load()) {
            QThread::msleep(1);
        }
        return std::unique_ptr<GPSTransport>{};
    };
    bool handled = false;
    QObject observer;
    connect(enabled, &Fact::rawValueChanged, &observer, [&] {
        if (std::exchange(handled, true)) {
            return;
        }
        // Turning auto-connect off is published after the manual connection is installed.
        QVERIFY(receiver->hasReceiver());
        QVERIFY(ports.isPortReserved(QStringLiteral("/test/reentrant")));
        if (destroy) {
            receiver.reset();
        } else {
            receiver->disconnectGPS();
        }
    });
    QVERIFY(receiver->connectConfiguredGPS());
    QVERIFY(handled);
    QVERIFY(!enabled->rawValue().toBool());
    if (receiver) {
        QVERIFY(!receiver->hasReceiver());
        QVERIFY(!receiver->reconnecting());
    }
    QTRY_VERIFY_WITH_TIMEOUT(!ports.isPortReserved(QStringLiteral("/test/reentrant")), TestTimeout::mediumMs());
#endif
}

void GPSRtkTest::_failedOpenNeverConnects()
{
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    GPSRtk receiver;
    auto* facts = qobject_cast<GPSRTKFactGroup*>(receiver.gpsRtkFactGroup());
    QSignalSpy connected(facts->connected(), &Fact::rawValueChanged);
    expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open GPS receiver transport")));
    receiver.connectReceiver(GPSType::ublox, {});
    QVERIFY(!receiver.connected());
    QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
    QVERIFY(!receiver.connected());
    QVERIFY(connected.isEmpty());
    QCOMPARE(static_cast<int>(receiver._connectionError), static_cast<int>(GPSConnectionError::OpenFailed));
    verifyExpectedLogMessage();
}

void GPSRtkTest::_receiverPublishesGcsPosition()
{
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    auto gate = std::make_shared<BlockedOpen>();
    const auto releaseWorker = qScopeGuard([&]() { gate->release.release(); });
    GPSPositionService positions;
    GPSRtk receiver;
    receiver.setPositionService(&positions);
    receiver.connectReceiver(GPSType::ublox, blockedFactory(gate), QStringLiteral("serial:test-base"));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver._session.provider;
    GPSPositionReport report = fixReport(GPSFixQuality::RTKFixed);
    report.navigation.latitudeDegrees = 47.25;
    report.navigation.longitudeDegrees = 8.5;
    report.navigation.altitudeMslMeters = 450;
    report.navigation.horizontalAccuracyMeters = 0.02f;
    emit provider->positionUpdate(report);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    // Registration waits until the receiver is configured.
    QCOMPARE(positions.selectedSource(), GPSPositionService::SelectedSource::None);

    emit provider->receiverReady();
    emit provider->positionUpdate(report);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(positions.selectedSource(), GPSPositionService::SelectedSource::Receiver);
    QCOMPARE(positions.gcsPosition().latitude(), 47.25);
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), qreal(0.02f));
    QCOMPARE(receiver.gpsRtkFactGroup()->fixType()->rawValue().toInt(), static_cast<int>(GPSFixQuality::RTKFixed));

    receiver.disconnectGPS();
    QVERIFY(!positions.gcsPosition().isValid());
    QCOMPARE(positions.selectedSource(), GPSPositionService::SelectedSource::None);
    QVERIFY(!receiver.acceptedPositionObservation(GPSObservation::PositionUse::Gga));
    emit provider->positionUpdate(report);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(!receiver.acceptedPositionObservation(GPSObservation::PositionUse::Gga));
}

void GPSRtkTest::_fixedBasePositionIsGcsPosition()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), settings->baseReceiverManufacturers()->rawValue());
    saved.setFactValue(settings->useFixedBasePosition(), static_cast<int>(BaseModeDefinition::Mode::BaseFixed));
    saved.setFactValue(settings->fixedBasePositionLatitude(), 47.5);
    saved.setFactValue(settings->fixedBasePositionLongitude(), 8.25);
    saved.setFactValue(settings->fixedBasePositionAltitude(), 480.0);
    saved.setFactValue(settings->fixedBasePositionAccuracy(), 0.0);
    auto gate = std::make_shared<BlockedOpen>();
    const auto releaseWorker = qScopeGuard([&]() { gate->release.release(); });
    GPSPositionService positions;
    GPSRtk receiver;
    receiver.setPositionService(&positions);
    QVERIFY(receiver.connectReceiver(GPSType::ublox, blockedFactory(gate)));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver._session.provider;
    QCOMPARE(receiver.basePosition(), QGeoCoordinate(47.5, 8.25));
    QVERIFY(receiver.basePositionFinal());
    emit provider->receiverReady();
    // Fixed-mode receivers report only a time fix.
    emit provider->positionUpdate(fixReport(GPSFixQuality::NoFix));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(positions.selectedSource(), GPSPositionService::SelectedSource::Receiver);
    QCOMPARE(positions.gcsPosition(), QGeoCoordinate(47.5, 8.25));
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), 0.01);
    const auto remoteId = receiver.acceptedPositionObservation(GPSObservation::PositionUse::RemoteID);
    QVERIFY(remoteId);
    QCOMPARE(remoteId->altitudeDatum, GPSAltitudeDatum::Ellipsoid);
    QCOMPARE(remoteId->position.coordinate().altitude(), 480.0);
    const auto gga = receiver.acceptedPositionObservation(GPSObservation::PositionUse::Gga);
    QVERIFY(gga);
    QVERIFY(gga->altitudeDatum != GPSAltitudeDatum::MeanSeaLevel);
    QCOMPARE(receiver.gpsRtkFactGroup()->fixType()->rawValue().toInt(), static_cast<int>(GPSFixQuality::NoFix));
    QSignalSpy baseChanges(&receiver, &GPSRtk::basePositionChanged);
    receiver.disconnectGPS();
    QCOMPARE(baseChanges.size(), 1);
    QVERIFY(!receiver.basePosition().isValid());
}

void GPSRtkTest::_receiverIntegrityFacts()
{
    GPSRtk receiver;
    auto* facts = receiver.gpsRtkFactGroup();
    GPSPositionReport report = fixReport(GPSFixQuality::Fix3D);
    report.integrity.jamming.state = GPSIntegrityReport::JammingState::Warning;
    report.integrity.spoofing.state = GPSIntegrityReport::SpoofingState::Indicated;
    receiver._positionUpdate(report);
    QCOMPARE(facts->jammingState()->rawValue().toInt(), 2);
    QCOMPARE(facts->spoofingState()->rawValue().toInt(), 2);
    QCOMPARE(facts->jammingState()->enumStringValue(), QStringLiteral("Warning"));
    QVERIFY(facts->interferenceWarning());
    report.integrity = {};
    receiver._positionUpdate(report);
    QCOMPARE(facts->jammingState()->rawValue().toInt(), 0);
    QVERIFY(!facts->interferenceWarning());
    report.integrity.jamming.state = GPSIntegrityReport::JammingState::Ok;
    report.integrity.spoofing.state = GPSIntegrityReport::SpoofingState::None;
    receiver._positionUpdate(report);
    QVERIFY(!facts->interferenceWarning());
    QSignalSpy interference(facts, &GPSRTKFactGroup::interferenceWarningChanged);
    report.integrity.jamming.state = GPSIntegrityReport::JammingState::Critical;
    receiver._positionUpdate(report);
    QVERIFY(facts->interferenceWarning());
    QVERIFY(!interference.isEmpty());
    receiver.disconnectGPS();
    QCOMPARE(facts->jammingState()->rawValue().toInt(), 0);
    QCOMPARE(facts->spoofingState()->rawValue().toInt(), 0);
    QVERIFY(!facts->interferenceWarning());
}

void GPSRtkTest::_surveyedBasePositionIsGcsPosition()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), settings->baseReceiverManufacturers()->rawValue());
    saved.setFactValue(settings->useFixedBasePosition(), static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn));
    saved.setFactValue(settings->surveyInAccuracyLimit(), 2.0);
    auto gate = std::make_shared<BlockedOpen>();
    const auto releaseWorker = qScopeGuard([&]() { gate->release.release(); });
    GPSPositionService positions;
    GPSRtk receiver;
    receiver.setPositionService(&positions);
    QVERIFY(receiver.connectReceiver(GPSType::ublox, blockedFactory(gate)));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver._session.provider;
    const auto deliver = [&](const GPSPositionReport& report) {
        emit provider->positionUpdate(report);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    };
    GPSPositionReport navigating = fixReport(GPSFixQuality::Fix3D);
    navigating.navigation.latitudeDegrees = 10;
    navigating.navigation.longitudeDegrees = 20;
    navigating.navigation.horizontalAccuracyMeters = 3;
    emit provider->receiverReady();
    deliver(navigating);
    QCOMPARE(positions.gcsPosition().latitude(), 10.0);
    QVERIFY(!receiver.basePosition().isValid());

    GPSSurveyReport survey;
    survey.active = false;
    survey.valid = true;
    survey.position = {.latitudeDegrees = 11, .longitudeDegrees = 21, .altitudeMeters = 400};
    survey.meanAccuracyMeters = 1.5;
    emit provider->surveyInStatus(survey);
    deliver(fixReport(GPSFixQuality::NoFix));
    QCOMPARE(positions.gcsPosition(), QGeoCoordinate(11, 21));
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), 1.5);
    QCOMPARE(receiver.basePosition(), QGeoCoordinate(11, 21));
    QVERIFY(receiver.basePositionFinal());

    survey.meanAccuracyMeters.reset();
    emit provider->surveyInStatus(survey);
    deliver(fixReport(GPSFixQuality::NoFix));
    QCOMPARE(positions.gcsPositionHorizontalAccuracy(), 2.0);

    survey.valid = false;
    survey.active = true;
    emit provider->surveyInStatus(survey);
    deliver(fixReport(GPSFixQuality::NoFix));
    QVERIFY(!positions.gcsPosition().isValid());
    // The survey-in mean so far still locates the base.
    QCOMPARE(receiver.basePosition(), QGeoCoordinate(11, 21));
    QVERIFY(!receiver.basePositionFinal());
}

void GPSRtkTest::_retiredWorkerCannotUpdateReplacement()
{
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    auto firstGate = std::make_shared<BlockedOpen>();
    auto secondGate = std::make_shared<BlockedOpen>();
    GPSCorrectionManager corrections;
    GPSRtk receiver;
    receiver.setCorrectionManager(&corrections);
    QSignalSpy routed(&corrections._router, &GPSCorrectionRouter::frameRouted);
    const auto releaseWorkers = qScopeGuard([&]() {
        firstGate->release.release();
        secondGate->release.release();
    });
    receiver.connectReceiver(GPSType::ublox, blockedFactory(firstGate), QStringLiteral("serial:test-base"));
    QTRY_VERIFY_WITH_TIMEOUT(firstGate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> first = receiver._session.provider;
    auto* facts = qobject_cast<GPSRTKFactGroup*>(receiver.gpsRtkFactGroup());
    QVERIFY(!receiver.connected());
    emit first->receiverReady();
    GPSSurveyReport survey{};
    survey.valid = true;
    survey.active = true;
    survey.position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500};
    survey.duration = std::chrono::seconds(4294967295LL);
    survey.meanAccuracyMeters = 1.5;
    emit first->surveyInStatus(survey);
    GPSSatelliteReport satellites;
    satellites.timestampUs = 1;
    satellites.inView = 2;
    satellites.used = 7;
    emit first->satelliteInfoUpdate(satellites);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver.connected());
    QVERIFY(facts->valid()->rawValue().toBool());
    QCOMPARE(facts->currentLatitude()->rawValue().toDouble(), 47.0);
    QCOMPARE(facts->currentLongitude()->rawValue().toDouble(), 8.0);
    QCOMPARE(facts->currentAltitude()->rawValue().toDouble(), 500.0);
    QCOMPARE(facts->currentAccuracy()->rawValue().toDouble(), 1.5);
    QCOMPARE(facts->currentDuration()->rawValue().toLongLong(), 4294967295LL);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), 2);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), 7);

    const auto frame = GpsTestHelpers::buildRtcmFrame(1005);
    emit first->RTCMDataUpdate(frame, GPSCorrectionFrame::monotonicNowMs());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(routed.size(), 1);
    const auto original = qvariant_cast<GPSCorrectionFrame>(routed[0][0]);
    QCOMPARE(original.source, GPSCorrectionSource::LocalReceiver);
    QCOMPARE(original.sourceInstance, QStringLiteral("serial:test-base"));
    QVERIFY(original.validated);
    auto* rtcm = corrections.rtcmMavlink();
    const auto bytesBefore = rtcm->totalBytesSent();
    QCOMPARE(bytesBefore, quint64(frame.size()));
    // Retirement must reject callbacks already in the GUI queue.
    satellites.used = 12;
    emit first->RTCMDataUpdate(frame, GPSCorrectionFrame::monotonicNowMs());
    emit first->surveyInStatus(survey);
    emit first->satelliteInfoUpdate(satellites);
    emit first->positionUpdate(fixReport(GPSFixQuality::Unknown));
    emit first->receiverReady();
    emit first->connectionError(GPSConnectionError::ConfigFailed,
                                QStringLiteral("Retired receiver configuration failure"));
    emit first->connectionError(GPSConnectionError::DeviceError);
    receiver.connectReceiver(GPSType::ublox, blockedFactory(secondGate), QStringLiteral("serial:test-base"));
    QVERIFY(!receiver.connected());
    QVERIFY(!facts->valid()->rawValue().toBool());
    QVERIFY(!facts->active()->rawValue().toBool());
    QVERIFY(qIsNaN(facts->currentLatitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(facts->currentAccuracy()->rawValue().toDouble()));
    QCOMPARE(facts->currentDuration()->rawValue().toInt(), 0);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), -1);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), -1);
    QTRY_VERIFY_WITH_TIMEOUT(secondGate->entered.available() > 0, TestTimeout::mediumMs());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(!receiver.connected());
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), -1);
    QCOMPARE(static_cast<int>(receiver._connectionError), static_cast<int>(GPSConnectionError::None));
    QVERIFY(receiver.errorMessage().isEmpty());
    QCOMPARE(rtcm->totalBytesSent(), bytesBefore);
    emit receiver._session.provider->receiverReady();
    const auto receivedAtMs = GPSCorrectionFrame::monotonicNowMs() - 10;
    emit receiver._session.provider->RTCMDataUpdate(frame, receivedAtMs);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver.connected());
    QCOMPARE(rtcm->totalBytesSent(), bytesBefore + frame.size());
    QCOMPARE(routed.size(), 2);
    const auto replacement = qvariant_cast<GPSCorrectionFrame>(routed[1][0]);
    QVERIFY(replacement.session != original.session);
    QCOMPARE(replacement.sourceInstance, original.sourceInstance);
    QCOMPARE(replacement.receivedAtMs, receivedAtMs);
    firstGate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(first.isNull(), TestTimeout::mediumMs());
    QVERIFY(firstGate->sawCancellation);
    QVERIFY(receiver.connected());

    expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GPS device error, connection lost")));
    const auto second = receiver._session.provider;
    emit second->connectionError(GPSConnectionError::DeviceError);
    emit second->RTCMDataUpdate(frame, GPSCorrectionFrame::monotonicNowMs());
    emit second->receiverReady();
    emit second->surveyInStatus(survey);
    emit second->satelliteInfoUpdate(satellites);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    verifyExpectedLogMessage();
    QVERIFY(!receiver.connected());
    QCOMPARE(facts->currentDuration()->rawValue().toLongLong(), 0);
    QCOMPARE(facts->numSatellites()->rawValue().toInt(), -1);
    QCOMPARE(facts->numSatellitesUsed()->rawValue().toInt(), -1);
    QVERIFY(corrections.sourceInstances().isEmpty());
    QCOMPARE(routed.size(), 2);
    QCOMPARE(rtcm->totalBytesSent(), bytesBefore + frame.size());

    secondGate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(second.isNull(), TestTimeout::mediumMs());
    QVERIFY(secondGate->sawCancellation);
    QVERIFY(!receiver.connected());
    QVERIFY(corrections.sourceInstances().isEmpty());
}

void GPSRtkTest::_workerCanOutliveManager()
{
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    auto gate = std::make_shared<BlockedOpen>();
    auto receiver = std::make_unique<GPSRtk>();
    const auto releaseWorker = qScopeGuard([&]() { gate->release.release(); });
    receiver->connectReceiver(GPSType::ublox, blockedFactory(gate));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver->_session.provider;
    emit provider->receiverReady();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver->connected());
    receiver.reset();
    QVERIFY(provider);
    QVERIFY(!provider->parent());
    gate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(provider.isNull(), TestTimeout::mediumMs());
    QVERIFY(gate->sawCancellation);
}

void GPSRtkTest::_receiverFramesAreValidated_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<bool>("expired");
    const auto good = GpsTestHelpers::buildRtcmFrame(1005);
    auto badCrc = good;
    badCrc.back() ^= 1;
    auto badHeader = good;
    badHeader[1] |= 0x80;
    QTest::newRow("valid") << good << true << false;
    QTest::newRow("bad-crc") << badCrc << false << false;
    QTest::newRow("reserved-header-bits") << badHeader << false << false;
    QTest::newRow("truncated") << good.first(good.size() - 1) << false << false;
    QTest::newRow("unframed") << QByteArrayLiteral("corrections") << false << false;
    QTest::newRow("expired-before-dequeue") << good << true << true;
}

void GPSRtkTest::_receiverFramesAreValidated()
{
    QFETCH(QByteArray, frame);
    QFETCH(bool, valid);
    QFETCH(bool, expired);
    TestFixtures::SettingsFixture saved;
    auto* manufacturer = SettingsManager::instance()->rtkSettings()->baseReceiverManufacturers();
    saved.setFactValue(manufacturer, manufacturer->rawValue());
    auto gate = std::make_shared<BlockedOpen>();
    GPSCorrectionManager corrections;
    GPSRtk receiver;
    receiver.setCorrectionManager(&corrections);
    const auto releaseWorker = qScopeGuard([&]() { gate->release.release(); });
    receiver.connectReceiver(GPSType::ublox, blockedFactory(gate));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    const auto receivedAtMs =
        GPSCorrectionFrame::monotonicNowMs() - (expired ? GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS : 0);
    QSignalSpy routed(&corrections._router, &GPSCorrectionRouter::frameRouted);
    emit receiver._session.provider->RTCMDataUpdate(frame, receivedAtMs);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    const auto stats = corrections.sourceDiagnostics()[static_cast<int>(GPSCorrectionSource::LocalReceiver)].toMap();
    QCOMPARE(stats.value(QStringLiteral("receivedFrames")).toULongLong(), 1);
    QCOMPARE(stats.value(QStringLiteral("validatedFrames")).toULongLong(), valid ? 1 : 0);
    QCOMPARE(routed.size(), valid && !expired ? 1 : 0);
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSent(), valid && !expired ? quint64(frame.size()) : 0);
    if (!routed.isEmpty()) {
        const auto correction = qvariant_cast<GPSCorrectionFrame>(routed[0][0]);
        QCOMPARE(correction.data, frame);
        QCOMPARE(correction.messageId, 1005);
        QVERIFY(correction.validated);
        QCOMPARE(correction.receivedAtMs, receivedAtMs);
    }
}

void GPSRtkTest::_runtimeSettingsDoNotRequireAppRestart_data()
{
    QTest::addColumn<QString>("name");
    for (const auto* name :
         {"baseReceiverManufacturers", "serialDevice", "serialBaudRate", "useFixedBasePosition",
          "surveyInAccuracyLimit", "surveyInMinObservationDuration", "receiverAveragingDuration",
          "fixedBasePositionLatitude", "fixedBasePositionLongitude", "fixedBasePositionAltitude",
          "fixedBasePositionAccuracy", "compactRtcmCorrections", "connectionType", "tcpHost", "tcpPort"}) {
        QTest::newRow(name) << QString::fromLatin1(name);
    }
}

void GPSRtkTest::_runtimeSettingsDoNotRequireAppRestart()
{
    QFETCH(QString, name);
    auto* settings = SettingsManager::instance()->rtkSettings();
    auto* fact = settings->property(name.toUtf8().constData()).value<Fact*>();
    QVERIFY(fact);
    QVERIFY(!fact->qgcRebootRequired());
    QVERIFY(!fact->vehicleRebootRequired());
}

void GPSRtkTest::_compactCorrectionsFollowReceiverSupport()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->useFixedBasePosition(), static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn));
    saved.setFactValue(settings->surveyInAccuracyLimit(), 2.0);
    saved.setFactValue(settings->surveyInMinObservationDuration(), 60);
    for (const bool compact : {false, true}) {
        saved.setFactValue(settings->compactRtcmCorrections(), compact);
        GPSReceiverConfig config;
        QVERIFY(GPSRtk::_receiverConfig(GPSType::ublox, settings, 115200, config).isEmpty());
        QCOMPARE(config.base.compactObservations, compact);
        // Receivers without MSM4 support ignore the hidden option instead of failing to connect.
        QVERIFY(GPSRtk::_receiverConfig(GPSType::septentrio, settings, 115200, config).isEmpty());
        QVERIFY(!config.base.compactObservations);
    }
}

void GPSRtkTest::_tcpPassiveConnection()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    auto* autoConnect = SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS();
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    saved.setFactValue(settings->baseReceiverManufacturers(), 7);
    saved.setFactValue(settings->connectionType(), GPSRtk::Tcp);
    saved.setFactValue(settings->tcpHost(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->tcpPort(), server.serverPort());
    saved.setFactValue(autoConnect, true);
    GPSCorrectionManager corrections;
    GPSRtk receiver;
    receiver.setCorrectionManager(&corrections);
    QVERIFY(receiver.connectConfiguredGPS());
    QVERIFY(!autoConnect->rawValue().toBool());
    const QString endpoint = QStringLiteral("127.0.0.1:%1").arg(server.serverPort());
    QCOMPARE(receiver.activeEndpoint(), endpoint);
    QCOMPARE(receiver._session.provider->_config.baudRate, TCPGPSTransport::FIXED_BAUDRATE);
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    QTcpSocket* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QTRY_VERIFY_WITH_TIMEOUT(receiver.connected(), TestTimeout::mediumMs());
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 20);
    QCOMPARE(peer->write(frame), frame.size());
    QTRY_COMPARE_WITH_TIMEOUT(corrections.sourceDiagnostics()[static_cast<int>(GPSCorrectionSource::LocalReceiver)]
                                  .toMap()
                                  .value(QStringLiteral("validatedFrames"))
                                  .toULongLong(),
                              1ULL, TestTimeout::mediumMs());
    const auto instances = corrections.sourceInstances();
    QCOMPARE(instances.size(), 1);
    QCOMPARE(instances.first().toMap().value(QStringLiteral("instanceId")).toString(),
             QStringLiteral("tcp:%1").arg(endpoint));
    receiver.disconnectConfiguredGPS();
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(receiver.activeEndpoint().isEmpty());
    QTRY_COMPARE_WITH_TIMEOUT(peer->state(), QAbstractSocket::UnconnectedState, TestTimeout::mediumMs());
}

void GPSRtkTest::_manualConnectionReconnectsAfterLoss()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    saved.setFactValue(settings->baseReceiverManufacturers(), 7);
    saved.setFactValue(settings->connectionType(), GPSRtk::Tcp);
    saved.setFactValue(settings->tcpHost(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->tcpPort(), server.serverPort());
    saved.setFactValue(SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS(), false);
    ignoreLogMessage("GPS.RTK.GPSRtk", QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
    ignoreLogMessage("GPS.Transport.TCPGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to connect to GPS receiver")));
    ignoreLogMessage("GPS.Driver.Protocols.Passive", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Receiver read failed")));
    GPSRtk receiver;
    auto* policy = receiver._connection;
    const auto tick = [policy]() {
        policy->update();
        return true;
    };
    QVERIFY(receiver.connectConfiguredGPS());
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    QTcpSocket* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QTRY_VERIFY_WITH_TIMEOUT(receiver.connected(), TestTimeout::mediumMs());
    QVERIFY(policy->_established);
    QVERIFY(!receiver.reconnecting());

    peer->abort();
    QTRY_VERIFY_WITH_TIMEOUT(receiver.reconnecting(), TestTimeout::mediumMs());
    QVERIFY(receiver.errorMessage().contains(QStringLiteral("Reconnecting")));
    policy->update();
    QVERIFY(!receiver.hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(tick() && server.hasPendingConnections(), TestTimeout::mediumMs());
    peer = server.nextPendingConnection();
    QVERIFY(peer);
    QTRY_VERIFY_WITH_TIMEOUT(receiver.connected(), TestTimeout::mediumMs());
    QVERIFY(receiver.errorMessage().isEmpty());
    // Automatic attempts never reuse one-use flash-save consent.
    QVERIFY(!receiver._session.provider->_config.allowPersistentChanges);
    QCOMPARE(policy->_retryDelayMs, 1000);

    server.close();
    peer->abort();
    QTRY_VERIFY_WITH_TIMEOUT(receiver.reconnecting(), TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(tick() && policy->_retryDelayMs >= 4000, TestTimeout::longMs());
    QTRY_VERIFY_WITH_TIMEOUT(receiver.reconnecting(), TestTimeout::mediumMs());
    settings->tcpPort()->setRawValue(server.serverPort() + 1);
    QVERIFY(!receiver.reconnecting());
    QVERIFY(policy->_retryDeadline.isForever());
}

void GPSRtkTest::_disconnectStopsReconnect()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    auto* autoConnect = SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS();
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    saved.setFactValue(settings->baseReceiverManufacturers(), 7);
    saved.setFactValue(settings->connectionType(), GPSRtk::Tcp);
    saved.setFactValue(settings->tcpHost(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->tcpPort(), server.serverPort());
    saved.setFactValue(autoConnect, false);
    ignoreLogMessage("GPS.RTK.GPSRtk", QtWarningMsg, QRegularExpression(QStringLiteral("session ended")));
    ignoreLogMessage("GPS.Transport.TCPGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to connect to GPS receiver")));
    ignoreLogMessage("GPS.Driver.Protocols.Passive", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Receiver read failed")));
    for (const bool viaAutoConnect : {false, true}) {
        GPSRtk receiver;
        QVERIFY(receiver.connectConfiguredGPS());
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
        QTcpSocket* peer = server.nextPendingConnection();
        QTRY_VERIFY_WITH_TIMEOUT(receiver.connected(), TestTimeout::mediumMs());
        server.pauseAccepting();
        peer->abort();
        QTRY_VERIFY_WITH_TIMEOUT(receiver.reconnecting(), TestTimeout::mediumMs());
        if (viaAutoConnect) {
            autoConnect->setRawValue(true);
        } else {
            receiver.disconnectConfiguredGPS();
            QVERIFY(receiver.errorMessage().isEmpty());
        }
        QVERIFY(!receiver.reconnecting());
        QVERIFY(receiver._connection->_retryDeadline.isForever());
        receiver._connection->update();
        QVERIFY(!receiver.hasReceiver());
        autoConnect->setRawValue(false);
        server.resumeAccepting();
        while (server.hasPendingConnections()) {
            delete server.nextPendingConnection();
        }
    }
}

void GPSRtkTest::_tcpConnectionErrors_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<bool>("validPort");
    QTest::addColumn<bool>("attempted");
    QTest::newRow("missing-host") << QString() << true << false;
    QTest::newRow("missing-port") << QStringLiteral("127.0.0.1") << false << false;
    QTest::newRow("refused") << QStringLiteral("127.0.0.1") << true << true;
}

void GPSRtkTest::_tcpConnectionErrors()
{
    QFETCH(QString, host);
    QFETCH(bool, validPort);
    QFETCH(bool, attempted);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    auto* autoConnect = SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS();
    quint16 port = 0;
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        port = server.serverPort();
    }
    saved.setFactValue(settings->baseReceiverManufacturers(), 7);
    saved.setFactValue(settings->connectionType(), GPSRtk::Tcp);
    saved.setFactValue(settings->tcpHost(), host);
    saved.setFactValue(settings->tcpPort(), validPort ? port : 0);
    saved.setFactValue(autoConnect, true);
    GPSRtk receiver;
    QCOMPARE(receiver.connectConfiguredGPS(), attempted);
    QCOMPARE(autoConnect->rawValue().toBool(), !attempted);
    if (attempted) {
        ignoreLogMessage("GPS.Transport.TCPGPSTransport", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Failed to connect to GPS receiver")));
        expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg, QRegularExpression(QStringLiteral("Failed to open")));
        QTRY_VERIFY_WITH_TIMEOUT(!receiver.hasReceiver(), TestTimeout::mediumMs());
        verifyExpectedLogMessage();
    }
    QVERIFY(!receiver.hasReceiver());
    QCOMPARE(receiver._connectionError, GPSConnectionError::OpenFailed);
    QVERIFY(!receiver.errorMessage().isEmpty());
}

void GPSRtkTest::_manufacturerIds_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<int>("receiverType");
    QTest::newRow("all-is-not-a-receiver") << 0 << -1;
    QTest::newRow("trimble") << 1 << 1;
    QTest::newRow("septentrio") << 2 << 2;
    QTest::newRow("femtomes") << 3 << 3;
    QTest::newRow("ublox") << 4 << 0;
    QTest::newRow("unicore") << 5 << 4;
    QTest::newRow("quectel") << 6 << 5;
    QTest::newRow("passive") << 7 << 6;
    QTest::newRow("invalid") << 8 << -1;
}

void GPSRtkTest::_manufacturerIds()
{
    QFETCH(int, manufacturer);
    QFETCH(int, receiverType);
    RTKSettings settings;
    GPSRtk receiver;
    const auto type = GPSRtk::typeForManufacturer(manufacturer);
    const auto values = settings.baseReceiverManufacturers()->enumValues();
    QCOMPARE(values.size(), 8);
    for (int id = 0; id < values.size(); ++id) {
        QCOMPARE(values[id].toInt(), id);
    }
    QCOMPARE(type.has_value(), receiverType >= 0);
    if (type) {
        QCOMPARE(static_cast<int>(*type), receiverType);
        QCOMPARE(GPSRtk::manufacturerForType(*type), manufacturer);
    }
    const auto caps = receiver.capabilitiesForManufacturer(manufacturer);
    QCOMPARE(caps.value(QStringLiteral("recognized")).toBool(), manufacturer < 8);
    QCOMPARE(caps.value(QStringLiteral("passive")).toBool(), manufacturer == 7);
    QCOMPARE(caps.value(QStringLiteral("rtkBase")).toBool(), manufacturer < 7);
    QCOMPARE(caps.value(QStringLiteral("receiverAveraging")).toBool(), manufacturer == 0 || manufacturer == 5);
    QCOMPARE(caps.value(QStringLiteral("surveyIn")).toBool(), manufacturer < 7 && manufacturer != 5);
}

void GPSRtkTest::_receiverSettingsMapping_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<int>("baseMode");
    QTest::addColumn<bool>("accepted");
    for (const int manufacturer : {4, 5, 6, 7}) {
        for (const int mode : {0, 1, 2}) {
            const bool accepted = manufacturer == 7 || mode == 1 || (mode == 2 ? manufacturer == 5 : manufacturer != 5);
            QTest::newRow(qPrintable(QStringLiteral("receiver-%1-mode-%2").arg(manufacturer).arg(mode)))
                << manufacturer << mode << accepted;
        }
    }
}

void GPSRtkTest::_receiverSettingsMapping()
{
    QFETCH(int, manufacturer);
    QFETCH(int, baseMode);
    QFETCH(bool, accepted);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->useFixedBasePosition(), baseMode);
    saved.setFactValue(settings->surveyInAccuracyLimit(), 1.75);
    saved.setFactValue(settings->surveyInMinObservationDuration(), 195);
    saved.setFactValue(settings->receiverAveragingDuration(), 321);
    saved.setFactValue(settings->fixedBasePositionLatitude(), 47.5);
    saved.setFactValue(settings->fixedBasePositionLongitude(), 8.25);
    saved.setFactValue(settings->fixedBasePositionAltitude(), 512.0);
    saved.setFactValue(settings->fixedBasePositionAccuracy(), 1.5);
    GPSReceiverConfig config;
    const auto type = GPSRtk::typeForManufacturer(manufacturer);
    QVERIFY(type);
    const auto error = GPSRtk::_receiverConfig(*type, settings, 230400, config);
    QCOMPARE(error.isEmpty(), accepted);
    QCOMPARE(config.baudRate, uint32_t(230400));
    QVERIFY(!config.allowPersistentChanges);
    if (manufacturer == 7) {
        QCOMPARE(config.role, GPSReceiverConfig::Role::Passive);
        QCOMPARE(config.base, GPSBaseStationConfig{});
    } else {
        QCOMPARE(config.role, GPSReceiverConfig::Role::RTKBase);
        QCOMPARE(std::holds_alternative<GPSBaseStationConfig::Fixed>(config.base.mode), baseMode == 1);
        if (baseMode == 1) {
            const auto& fixed = std::get<GPSBaseStationConfig::Fixed>(config.base.mode);
            QCOMPARE(fixed.position.latitudeDegrees, 47.5);
            QCOMPARE(fixed.position.longitudeDegrees, 8.25);
            QCOMPARE(fixed.position.altitudeMeters, 512.0f);
            QCOMPARE(fixed.accuracyMeters, 1.5f);
        } else if (baseMode == 2) {
            QVERIFY(std::holds_alternative<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode));
            QCOMPARE(std::get<GPSBaseStationConfig::ReceiverAveraging>(config.base.mode).maximumDurationSecs,
                     uint32_t(321));
        } else {
            QVERIFY(std::holds_alternative<GPSBaseStationConfig::SurveyIn>(config.base.mode));
            const auto& survey = std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode);
            QCOMPARE(survey.accuracyMeters, 1.75);
            QCOMPARE(survey.durationSecs, int64_t(195));
        }
    }
}

void GPSRtkTest::_invalidReceiverSettings_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<int>("mode");
    QTest::addColumn<uint>("averagingDuration");
    QTest::addColumn<uint>("baud");
    QTest::addColumn<bool>("accepted");
    QTest::newRow("unicore-needs-explicit-mode") << 5 << 0 << 60U << 115200U << false;
    QTest::newRow("quectel-rejects-averaging") << 6 << 2 << 60U << 115200U << false;
    QTest::newRow("unknown-mode") << 6 << 42 << 60U << 115200U << false;
    QTest::newRow("zero-averaging") << 5 << 2 << 0U << 115200U << false;
    QTest::newRow("excess-averaging") << 5 << 2 << 3601U << 115200U << false;
    QTest::newRow("minimum-averaging") << 5 << 2 << 1U << 115200U << true;
    QTest::newRow("maximum-averaging") << 5 << 2 << 3600U << 115200U << true;
    QTest::newRow("passive-needs-baud") << 7 << 0 << 60U << 0U << false;
    QTest::newRow("passive-ignores-stale-base-settings") << 7 << 42 << 0U << 115200U << true;
}

void GPSRtkTest::_invalidReceiverSettings()
{
    QFETCH(int, manufacturer);
    QFETCH(int, mode);
    QFETCH(uint, averagingDuration);
    QFETCH(uint, baud);
    QFETCH(bool, accepted);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->useFixedBasePosition(), mode);
    saved.setFactValue(settings->receiverAveragingDuration(), averagingDuration);
    GPSReceiverConfig config;
    const auto type = GPSRtk::typeForManufacturer(manufacturer);
    QVERIFY(type);
    QCOMPARE(GPSRtk::_receiverConfig(*type, settings, baud, config).isEmpty(), accepted);
    QCOMPARE(settings->receiverAveragingDuration()->rawMin().toUInt(), 1U);
    QCOMPARE(settings->receiverAveragingDuration()->rawMax().toUInt(), 3600U);
    QCOMPARE(settings->useFixedBasePosition()->enumValues(), (QVariantList{0, 1, 2}));
    if (!accepted) {
        GPSRtk receiver;
        bool opened = false;
        QVERIFY(!receiver.connectReceiver(
            *type,
            [&opened](const std::atomic_bool&) {
                opened = true;
                return std::unique_ptr<GPSTransport>{};
            },
            {}, baud));
        QVERIFY(!opened);
        QVERIFY(!receiver.hasReceiver());
        QVERIFY(!receiver.errorMessage().isEmpty());
        QCOMPARE(static_cast<int>(receiver._connectionError), static_cast<int>(GPSConnectionError::ConfigFailed));
    }
}

#ifndef QGC_NO_SERIAL_LINK
void GPSRtkTest::_explicitSerialSelectionAndDisconnect_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("unplug");
    QTest::addColumn<bool>("allowPersistentChanges");
    QTest::addColumn<uint>("baud");
    QTest::newRow("unicore-disconnect") << 5 << false << false << 115200U;
    QTest::newRow("unicore-auto-baud") << 5 << false << false << 0U;
    QTest::newRow("quectel-disconnect") << 6 << false << false << 115200U;
    QTest::newRow("quectel-one-use-consent") << 6 << false << true << 115200U;
    QTest::newRow("passive-disconnect") << 7 << false << false << 115200U;
    QTest::newRow("passive-unplug") << 7 << true << false << 115200U;
}

void GPSRtkTest::_explicitSerialSelectionAndDisconnect()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, unplug);
    QFETCH(bool, allowPersistentChanges);
    QFETCH(uint, baud);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    auto* autoConnect = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), manufacturer);
    saved.setFactValue(settings->useFixedBasePosition(), manufacturer == 5 ? 2 : 0);
    saved.setFactValue(settings->serialDevice(), QStringLiteral("/test/selected"));
    saved.setFactValue(settings->serialBaudRate(), baud);
    saved.setFactValue(autoConnect->autoConnectRTKGPS(), true);
    const QList<SerialPortManager::Port> inventory{
        {QStringLiteral("/test/unselected"), QStringLiteral("unselected"), QGCSerialPortInfo::BoardTypeUnknown,
         QStringLiteral("USB serial")},
        {QStringLiteral("/test/selected"), QStringLiteral("selected"), QGCSerialPortInfo::BoardTypeUnknown,
         QStringLiteral("USB serial")},
    };
    SerialPortManager ports(nullptr, [&] { return inventory; });
    GPSRtk receiver;
    receiver.setSerialPortManager(&ports);
    auto gate = std::make_shared<BlockedOpen>();
    QString openedDevice;
    receiver._serialTransportFactory = [gate, &openedDevice](const QString& device, const std::atomic_bool& stop) {
        openedDevice = device;
        return blockedFactory(gate)(stop);
    };
    const auto releaseWorker = qScopeGuard([&] { gate->release.release(); });
    QVERIFY(receiver.connectConfiguredGPS(allowPersistentChanges));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QCOMPARE(openedDevice, QStringLiteral("/test/selected"));
    QCOMPARE(receiver.activeEndpoint(), openedDevice);
    QCOMPARE(receiver.activeManufacturer(), manufacturer);
    QVERIFY(!autoConnect->autoConnectRTKGPS()->rawValue().toBool());
    QVERIFY(ports.isPortReserved(openedDevice));
    QVERIFY(!ports.isPortReserved(QStringLiteral("/test/unselected")));
    QVERIFY(!ports.reservePort(openedDevice));
    QPointer<GPSProvider> provider = receiver._session.provider;
    QCOMPARE(provider->_config.baudRate, baud);
    QSignalSpy receiverChanges(&receiver, &GPSRtk::receiverChanged);
    emit provider->receiverReady(QStringLiteral("ZED-F9P HPG 1.32"));
    GPSSurveyReport survey;
    survey.active = true;
    survey.valid = true;
    emit provider->surveyInStatus(survey);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver.connected());
    QCOMPARE(receiver.receiverIdentity(), QStringLiteral("ZED-F9P HPG 1.32"));
    QCOMPARE(receiverChanges.size(), 1);
    QCOMPARE(receiver.gpsRtkFactGroup()->active()->rawValue().toBool(), manufacturer != 7);
    QCOMPARE(receiver.gpsRtkFactGroup()->valid()->rawValue().toBool(), manufacturer != 7);
    gate->release.release();
    if (unplug) {
        emit ports.portsEnumerated({QStringLiteral("/test/unselected")});
        QVERIFY(receiver.errorMessage().contains(QStringLiteral("plugged back in")));
        QVERIFY(receiver.reconnecting());
        QVERIFY(receiver._connection->_waitingForPort);
    } else {
        receiver.disconnectConfiguredGPS();
        QVERIFY(receiver.errorMessage().isEmpty());
    }
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!receiver.connected());
    QVERIFY(receiver.receiverIdentity().isEmpty());
    QVERIFY(!receiver.gpsRtkFactGroup()->active()->rawValue().toBool());
    QVERIFY(!receiver.gpsRtkFactGroup()->valid()->rawValue().toBool());
    QTRY_VERIFY_WITH_TIMEOUT(provider.isNull(), TestTimeout::mediumMs());
    QVERIFY(ports.canReservePort(openedDevice));
    if (unplug) {
        // The returning port triggers an immediate attempt rather than waiting for the backoff.
        receiver.connectionPolicy()->update();
        QVERIFY(receiver.hasReceiver());
        QCOMPARE(receiver.activeEndpoint(), openedDevice);
        receiver.disconnectConfiguredGPS();
        QVERIFY(!receiver.reconnecting());
    }
}

void GPSRtkTest::_manualSerialErrors_data()
{
    QTest::addColumn<QString>("reason");
    for (const auto* reason : {"all", "missing", "empty", "bootloader", "busy", "single-port", "invalid-baud",
                               "passive-auto-baud", "unsupported-mode"}) {
        QTest::newRow(reason) << QString::fromLatin1(reason);
    }
}

void GPSRtkTest::_manualSerialErrors()
{
    QFETCH(QString, reason);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    auto* autoConnect = SettingsManager::instance()->autoConnectSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), reason == QStringLiteral("all")                 ? 0
                                                              : reason == QStringLiteral("passive-auto-baud") ? 7
                                                                                                              : 5);
    saved.setFactValue(settings->useFixedBasePosition(), reason == QStringLiteral("unsupported-mode") ? 0 : 2);
    saved.setFactValue(settings->serialDevice(),
                       reason == QStringLiteral("empty") ? QString() : QStringLiteral("/test/selected"));
    saved.setFactValue(settings->serialBaudRate(), reason == QStringLiteral("invalid-baud")        ? 1000
                                                   : reason == QStringLiteral("passive-auto-baud") ? 0
                                                                                                   : 115200);
    saved.setFactValue(autoConnect->autoConnectRTKGPS(), false);
    SerialPortManager::Port port{
        QStringLiteral("/test/selected"), QStringLiteral("selected"), QGCSerialPortInfo::BoardTypeUnknown, {}};
    port.bootloader = reason == QStringLiteral("bootloader");
    SerialPortManager ports(nullptr, [&] {
        return reason == QStringLiteral("missing") ? QList<SerialPortManager::Port>{}
                                                   : QList<SerialPortManager::Port>{port};
    });
    SerialPortManager::ReservationPtr reservation;
    if (reason == QStringLiteral("busy")) {
        reservation = ports.reservePort(port.systemLocation);
    } else if (reason == QStringLiteral("single-port")) {
        ports.setSinglePortOnly(true);
        (void) ports.availablePorts();
        reservation = ports.reservePort(QStringLiteral("/test/mavlink"));
    }
    GPSRtk receiver;
    receiver.setSerialPortManager(&ports);
    std::atomic_bool opened = false;
    receiver._serialTransportFactory = [&opened](const QString&, const std::atomic_bool&) {
        opened = true;
        return std::unique_ptr<GPSTransport>{};
    };
    QVERIFY(!receiver.connectConfiguredGPS());
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!receiver.connected());
    QVERIFY(!receiver.errorMessage().isEmpty());
    QVERIFY(!opened);
    QVERIFY(static_cast<int>(receiver._connectionError) != 0);
    if (reservation) {
        QVERIFY(ports.isPortReserved(reservation->systemLocation));
    } else {
        QVERIFY(ports.canReservePort(port.systemLocation));
    }
    QVERIFY(!receiver._connectGPS(port.systemLocation, QStringLiteral("USB serial"), 115200));
    QVERIFY(!opened);
}

void GPSRtkTest::_serialReservationSurvivesDelayedStop()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), 7);
    SerialPortManager ports(nullptr, [] { return QList<SerialPortManager::Port>{}; });
    auto gate = std::make_shared<BlockedOpen>();
    GPSRtk receiver;
    receiver.setSerialPortManager(&ports);
    receiver._serialTransportFactory = [gate](const QString&, const std::atomic_bool& stop) {
        return blockedFactory(gate)(stop);
    };
    const auto releaseWorker = qScopeGuard([&] { gate->release.release(); });
    QVERIFY(receiver._connectGPS(QStringLiteral("/test/selected"), QStringLiteral("passive"), 115200));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver._session.provider;
    emit provider->receiverReady();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(receiver.connected());
    bool heartbeat = false;
    QTimer::singleShot(0, &receiver, [&heartbeat] { heartbeat = true; });
    QElapsedTimer retirementTime;
    retirementTime.start();
    receiver.disconnectGPS();
    QVERIFY2(retirementTime.elapsed() < 500, "Retiring a worker must not wait for its transport on the GUI thread");
    QVERIFY(!receiver.hasReceiver());
    QVERIFY(!receiver.connected());
    QTRY_VERIFY_WITH_TIMEOUT(heartbeat, TestTimeout::shortMs());
    QVERIFY(provider && provider->isRunning());
    QVERIFY(ports.isPortReserved(QStringLiteral("/test/selected")));
    gate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(provider.isNull(), TestTimeout::mediumMs());
    QVERIFY(ports.canReservePort(QStringLiteral("/test/selected")));
}

namespace {
struct PassiveTransportState
{
    std::atomic_uint baud = 0;
    std::atomic_uint baudChanges = 0;
    std::atomic_uint writes = 0;
    QSemaphore reading;
    QSemaphore releaseRead;
};

class PassiveTestTransport : public ScriptedGPSTransport
{
public:
    PassiveTestTransport(const std::atomic_bool& stop, std::shared_ptr<PassiveTransportState> state)
        : ScriptedGPSTransport(stop)
        , _state(std::move(state))
    {}

protected:
    std::optional<bool> handleBaudrate(unsigned baud) override
    {
        _state->baud = baud;
        ++_state->baudChanges;
        return true;
    }

    std::optional<GPSReadResult> handleRead(uint8_t*, int, int) override
    {
        _state->reading.release();
        _state->releaseRead.acquire();
        return GPSReadResult{GPSReadStatus::Cancelled};
    }

    std::optional<GPSWriteResult> handleWrite(const QByteArray&, QDeadlineTimer) override
    {
        ++_state->writes;
        return GPSWriteResult{GPSWriteStatus::Error};
    }

private:
    std::shared_ptr<PassiveTransportState> _state;
};
}  // namespace

void GPSRtkTest::_manualPassiveBaudPreserved_data()
{
    QTest::addColumn<uint>("baud");
    QTest::newRow("minimum") << 1200U;
    QTest::newRow("usb-serial") << 230400U;
    QTest::newRow("maximum") << 4000000U;
}

void GPSRtkTest::_manualPassiveBaudPreserved()
{
    QFETCH(uint, baud);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), 7);
    saved.setFactValue(settings->useFixedBasePosition(), 2);
    saved.setFactValue(settings->serialDevice(), QStringLiteral("/test/passive"));
    saved.setFactValue(settings->serialBaudRate(), baud);
    saved.setFactValue(SettingsManager::instance()->autoConnectSettings()->autoConnectRTKGPS(), false);
    SerialPortManager ports(nullptr, [] {
        return QList<SerialPortManager::Port>{
            {QStringLiteral("/test/passive"), QStringLiteral("passive"), QGCSerialPortInfo::BoardTypeUnknown, {}}};
    });
    auto state = std::make_shared<PassiveTransportState>();
    GPSRtk receiver;
    receiver.setSerialPortManager(&ports);
    receiver._serialTransportFactory = [state](const QString&, const std::atomic_bool& stop) {
        return std::make_unique<PassiveTestTransport>(stop, state);
    };
    const auto releaseWorker = qScopeGuard([&] { state->releaseRead.release(); });
    QVERIFY(receiver.connectConfiguredGPS());
    QTRY_VERIFY_WITH_TIMEOUT(receiver.connected() && state->reading.available() > 0, TestTimeout::mediumMs());
    QCOMPARE(state->baud.load(), baud);
    QCOMPARE(state->baudChanges.load(), 1U);
    QCOMPARE(state->writes.load(), 0U);
    QVERIFY(!receiver.gpsRtkFactGroup()->active()->rawValue().toBool());
    QVERIFY(!receiver.gpsRtkFactGroup()->valid()->rawValue().toBool());
    QVERIFY(ports.isPortReserved(QStringLiteral("/test/passive")));
    state->releaseRead.release();
    receiver.disconnectConfiguredGPS();
    QVERIFY(!receiver.hasReceiver());
    QTRY_VERIFY_WITH_TIMEOUT(ports.canReservePort(QStringLiteral("/test/passive")), TestTimeout::mediumMs());
}
#endif

void GPSRtkTest::_persistentConsentMapping_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("allowPersistentChanges");
    for (const int manufacturer : {4, 5, 6, 7}) {
        for (const bool allow : {false, true}) {
            QTest::newRow(qPrintable(QStringLiteral("receiver-%1-consent-%2").arg(manufacturer).arg(allow)))
                << manufacturer << allow;
        }
    }
}

void GPSRtkTest::_persistentConsentMapping()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, allowPersistentChanges);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->useFixedBasePosition(), 1);
    saved.setFactValue(settings->fixedBasePositionLatitude(), 47.5);
    saved.setFactValue(settings->fixedBasePositionLongitude(), 8.25);
    saved.setFactValue(settings->fixedBasePositionAltitude(), 512.0);
    saved.setFactValue(settings->fixedBasePositionAccuracy(), 1.5);
    const auto type = GPSRtk::typeForManufacturer(manufacturer);
    QVERIFY(type);
    GPSReceiverConfig config;
    const auto diagnostic = GPSRtk::_receiverConfig(*type, settings, 115200, config, allowPersistentChanges);
    QCOMPARE(diagnostic.isEmpty(), !allowPersistentChanges || manufacturer == 6);
    QCOMPARE(config.allowPersistentChanges, allowPersistentChanges);
    QVERIFY(GPSRtk::_receiverConfig(*type, settings, 115200, config).isEmpty());
    QVERIFY(!config.allowPersistentChanges);
}

void GPSRtkTest::_configurationDiagnosticRetained_data()
{
    QTest::addColumn<QString>("detail");
    QTest::newRow("provisioning-mismatch") << QStringLiteral("Requested base settings differ from receiver readback.");
    QTest::newRow("possibly-persisted") << QStringLiteral("Settings may have been saved, but reconnect failed.");
    QTest::newRow("empty-fallback") << QString();
}

void GPSRtkTest::_configurationDiagnosticRetained()
{
    QFETCH(QString, detail);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), 6);
    saved.setFactValue(settings->useFixedBasePosition(), 0);
    auto gate = std::make_shared<BlockedOpen>();
    GPSRtk receiver;
    const auto releaseWorker = qScopeGuard([&] { gate->release.release(); });
    QVERIFY(receiver.connectReceiver(GPSType::quectel, blockedFactory(gate), {}, 115200, true));
    QTRY_VERIFY_WITH_TIMEOUT(gate->entered.available() > 0, TestTimeout::mediumMs());
    QPointer<GPSProvider> provider = receiver._session.provider;
    receiver._setError(GPSConnectionError::ConfigFailed, QStringLiteral("An earlier configuration error"));
    QSignalSpy messages(&receiver, &GPSRtk::errorMessageChanged);
    emit provider->connectionError(GPSConnectionError::ConfigFailed, detail);
    expectLogMessage("GPS.RTK.GPSRtk", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GPS receiver did not accept configuration")));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    verifyExpectedLogMessage();
    QCOMPARE(messages.size(), 1);
    QVERIFY(!receiver.connected());
    QCOMPARE(static_cast<int>(receiver._connectionError), static_cast<int>(GPSConnectionError::ConfigFailed));
    QVERIFY(!receiver.errorMessage().isEmpty());
    if (!detail.isEmpty()) {
        QVERIFY(receiver.errorMessage().contains(detail));
    } else {
        QCOMPARE(receiver.errorMessage(),
                 GPSRtk::tr("Receiver configuration failed. Check the receiver type, baud rate, and base mode."));
    }
    const QString message = receiver.errorMessage();
    gate->release.release();
    QTRY_VERIFY_WITH_TIMEOUT(provider.isNull(), TestTimeout::mediumMs());
    QCOMPARE(receiver.errorMessage(), message);
}

void GPSRtkTest::_qmlConsentIsOneUse()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->baseReceiverManufacturers(), 6);
    saved.setFactValue(settings->useFixedBasePosition(), 0);
    saved.setFactValue(settings->serialDevice(), QStringLiteral("/test/absent"));
    auto* receiver = GPSManager::instance()->gpsRtk();
    QVERIFY(!receiver->hasReceiver());
    const auto originalError = receiver->_connectionError;
    const QString originalMessage = receiver->errorMessage();
#ifndef QGC_NO_SERIAL_LINK
    SerialPortManager ports(nullptr, [] { return QList<SerialPortManager::Port>{}; });
    const auto originalPorts = receiver->_serialPorts;
    receiver->setSerialPortManager(&ports);
#endif
    const auto restore = qScopeGuard([&] {
        receiver->_setError(originalError, originalMessage);
#ifndef QGC_NO_SERIAL_LINK
        receiver->setSerialPortManager(originalPorts);
#endif
    });
    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/QGroundControl/Toolbar/GPSIndicatorPage.qml")));
    QTRY_VERIFY_WITH_TIMEOUT(!component.isLoading(), TestTimeout::mediumMs());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QVERIFY(root->setProperty("expanded", true));
    auto* checkbox = root->findChild<QObject*>(QStringLiteral("rtkPersistentChangesCheckBox"));
    QVERIFY(checkbox);
    QVERIFY(!checkbox->property("checked").toBool());
    QCOMPARE(checkbox->property("visible").toBool(), receiver->serialSupported());
    QVERIFY(root->setProperty("_allowPersistentChanges", true));
    QVERIFY(checkbox->property("checked").toBool());
    settings->serialDevice()->setRawValue(QStringLiteral("/test/other"));
    QVERIFY(!root->property("_allowPersistentChanges").toBool());
    QVERIFY(root->setProperty("_allowPersistentChanges", true));
    settings->useFixedBasePosition()->setRawValue(1);
    QVERIFY(!root->property("_allowPersistentChanges").toBool());
    QVERIFY(root->setProperty("_allowPersistentChanges", true));
    settings->baseReceiverManufacturers()->setRawValue(5);
    QVERIFY(!root->property("_allowPersistentChanges").toBool());
    QVERIFY(!checkbox->property("visible").toBool());
    settings->baseReceiverManufacturers()->setRawValue(6);
    QVERIFY(root->setProperty("_allowPersistentChanges", true));
    QQmlExpression attempt(qmlContext(root.get()), root.get(), QStringLiteral("connectSelectedReceiver()"));
    QVERIFY(!attempt.evaluate().toBool());
    QVERIFY2(!attempt.hasError(), qPrintable(attempt.error().toString()));
    QVERIFY(!root->property("_allowPersistentChanges").toBool());
    QVERIFY(!receiver->hasReceiver());
    QVERIFY(!receiver->errorMessage().isEmpty());
    QVERIFY(root->setProperty("_allowPersistentChanges", true));
    root.reset(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QVERIFY(!root->property("_allowPersistentChanges").toBool());
}
