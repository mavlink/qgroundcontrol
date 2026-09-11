#include "GPSBaseStationStateTest.h"

#include <QtCore/QPointer>

#include <memory>

#include "GPSBaseReferenceSave.h"
#include "GPSBaseStationState.h"
#include "GPSReceiverSession.h"
#include "RTKSettings.h"

namespace {
GPSSurveyInStatus validSurvey()
{
    return {.latitude = 47.5,
            .longitude = 8.5,
            .altitude = 500,
            .meanAccuracyMM = 250,
            .durationSecs = 180,
            .valid = true,
            .active = true};
}
}  // namespace

void GPSBaseStationStateTest::_attachReceiver(GPSReceiverSession& session, GPSReceiverConfig::Role role,
                                              GPSReceiverCapabilities::Support support)
{
    session.stop();
    GPSReceiverProfile profile;
    profile.receiver.role = role;
    session._attempt = {++session._generation,
                        std::make_shared<const GPSReceiverProfile>(profile),
                        GPSReceiverAttempt::Phase::Ready,
                        GPSConnectionError::None,
                        {}};
    session._capabilities = GPSReceiverCapabilities::forType(GPSType::u_blox);
    session._capabilities.rtkBase = support;
    session._provider = new GPSProvider({}, GPSType::u_blox, session.config(), {}, &session);
    emit session.receiverTypeChanged(GPSType::u_blox);
}

void GPSBaseStationStateTest::_surveyRoleGating_data()
{
    QTest::addColumn<bool>("base");
    QTest::addColumn<GPSReceiverCapabilities::Support>("support");
    QTest::addColumn<bool>("hasReceiver");
    QTest::addColumn<bool>("accepted");
    for (bool base : {false, true}) {
        for (auto support : {GPSReceiverCapabilities::Support::Unknown, GPSReceiverCapabilities::Support::Unsupported,
                             GPSReceiverCapabilities::Support::Supported}) {
            for (bool hasReceiver : {false, true}) {
                const QByteArray name = QByteArray(base ? "base-" : "position-") +
                                        QByteArray::number(static_cast<int>(support)) +
                                        (hasReceiver ? "-present" : "-absent");
                QTest::newRow(name.constData())
                    << base << support << hasReceiver
                    << (base && hasReceiver && support != GPSReceiverCapabilities::Support::Unsupported);
            }
        }
    }
}

void GPSBaseStationStateTest::_surveyRoleGating()
{
    QFETCH(bool, base);
    QFETCH(GPSReceiverCapabilities::Support, support);
    QFETCH(bool, hasReceiver);
    QFETCH(bool, accepted);
    GPSReceiverSession session;
    GPSReceiverState receiverState(session);
    GPSBaseStationFactGroup facts;
    GPSBaseStationState state(receiverState, facts);
    _attachReceiver(session, base ? GPSReceiverConfig::Role::RTKBase : GPSReceiverConfig::Role::Position, support);
    if (!hasReceiver) {
        session.stop();
    }
    emit session.surveyInReceived(validSurvey());
    QCOMPARE(facts.valid()->rawValue().toBool(), accepted);
    QCOMPARE(facts.active()->rawValue().toBool(), accepted);
    QCOMPARE(facts.currentDuration()->rawValue().toInt(), accepted ? 180 : 0);
    if (accepted) {
        QCOMPARE(facts.currentAccuracy()->rawValue().toDouble(), 0.25);
        QCOMPARE(facts.currentLatitude()->rawValue().toDouble(), 47.5);
        QCOMPARE(facts.currentLongitude()->rawValue().toDouble(), 8.5);
        QCOMPARE(facts.currentAltitude()->rawValue().toDouble(), 500.0);
    } else {
        QVERIFY(qIsNaN(facts.currentLatitude()->rawValue().toDouble()));
    }
    QCOMPARE(facts.factNames().size(), 7);
    QVERIFY(!facts.factNames().contains(QStringLiteral("connected")));
    QVERIFY(!facts.factNames().contains(QStringLiteral("lastError")));
    QVERIFY(!facts.factNames().contains(QStringLiteral("numSatellites")));
}

void GPSBaseStationStateTest::_roleChangeAndDisconnectReset()
{
    GPSReceiverSession session;
    GPSReceiverState receiverState(session);
    GPSBaseStationFactGroup facts;
    GPSBaseStationState state(receiverState, facts);
    _attachReceiver(session, GPSReceiverConfig::Role::RTKBase, GPSReceiverCapabilities::Support::Unknown);
    emit session.surveyInReceived(validSurvey());
    QVERIFY(facts.valid()->rawValue().toBool());
    _attachReceiver(session, GPSReceiverConfig::Role::Position, GPSReceiverCapabilities::Support::Supported);
    emit session.surveyInReceived(validSurvey());
    QVERIFY(!facts.valid()->rawValue().toBool());
    QVERIFY(!facts.active()->rawValue().toBool());
    QVERIFY(qIsNaN(facts.currentAccuracy()->rawValue().toDouble()));
    QVERIFY(qIsNaN(facts.currentLatitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(facts.currentLongitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(facts.currentAltitude()->rawValue().toDouble()));

    _attachReceiver(session, GPSReceiverConfig::Role::RTKBase, GPSReceiverCapabilities::Support::Supported);
    emit session.surveyInReceived(validSurvey());
    session._capabilities.rtkBase = GPSReceiverCapabilities::Support::Unsupported;
    emit session.capabilitiesUpdated(session.capabilities());
    QVERIFY(!facts.valid()->rawValue().toBool());
    emit session.surveyInReceived(validSurvey());
    QCOMPARE(facts.currentDuration()->rawValue().toInt(), 0);
    session.stop();
    emit session.surveyInReceived(validSurvey());
    QVERIFY(!facts.valid()->rawValue().toBool());
}

void GPSBaseStationStateTest::_roleChangeDuringSurveyUpdate()
{
    GPSReceiverSession session;
    GPSReceiverState receiverState(session);
    GPSBaseStationFactGroup facts;
    GPSBaseStationState state(receiverState, facts);
    _attachReceiver(session, GPSReceiverConfig::Role::RTKBase, GPSReceiverCapabilities::Support::Supported);
    bool switched = false;
    connect(facts.currentDuration(), &Fact::rawValueChanged, this, [&]() {
        if (!switched) {
            switched = true;
            _attachReceiver(session, GPSReceiverConfig::Role::Position, GPSReceiverCapabilities::Support::Supported);
        }
    });
    emit session.surveyInReceived(validSurvey());
    QVERIFY(switched);
    QCOMPARE(session.config().role, GPSReceiverConfig::Role::Position);
    QVERIFY(!facts.valid()->rawValue().toBool());
    QVERIFY(!facts.active()->rawValue().toBool());
    QCOMPARE(facts.currentDuration()->rawValue().toInt(), 0);
    QVERIFY(qIsNaN(facts.currentLatitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(facts.currentAccuracy()->rawValue().toDouble()));
}

void GPSBaseStationStateTest::_presentationDestructionKeepsSession()
{
    GPSReceiverSession session;
    GPSReceiverState receiverState(session);
    _attachReceiver(session, GPSReceiverConfig::Role::RTKBase, GPSReceiverCapabilities::Support::Supported);
    const QPointer<GPSProvider> worker = session._provider;
    GPSBaseStationFactGroup facts;
    auto state = std::make_unique<GPSBaseStationState>(receiverState, facts);
    connect(facts.currentDuration(), &Fact::rawValueChanged, this, [&]() { state.reset(); });
    emit session.surveyInReceived(validSurvey());
    QVERIFY(!state);
    QVERIFY(worker);
    QVERIFY(session.hasReceiver());
    session.stop();
    QVERIFY(!worker);
}

void GPSBaseStationStateTest::_referenceMetadata()
{
    GPSReceiverSession session;
    GPSReceiverState receiverState(session);
    GPSBaseStationFactGroup facts;
    GPSBaseStationState state(receiverState, facts);
    _attachReceiver(session, GPSReceiverConfig::Role::RTKBase, GPSReceiverCapabilities::Support::Supported);
    auto survey = validSurvey();
    survey.meanAccuracyMM.reset();
    survey.altitudeDatum = GPSObservation::AltitudeDatum::Ellipsoid;
    survey.sessionId = session.sessionId();
    emit session.surveyInReceived(survey);
    QVERIFY(state.reference().isValid());
    QVERIFY(!state.reference().accuracyMeters);
    QVERIFY(qIsNaN(facts.currentAccuracy()->rawValue().toDouble()));
    QCOMPARE(state.reference().observation.altitudeDatum, GPSObservation::AltitudeDatum::Ellipsoid);
    survey.meanAccuracyMM = 0;
    emit session.surveyInReceived(survey);
    QCOMPARE(state.reference().accuracyMeters.value(), 0.0);
    QCOMPARE(facts.currentAccuracy()->rawValue().toDouble(), 0.0);
    survey.sessionId += 10;
    survey.latitude = 20;
    emit session.surveyInReceived(survey);
    QCOMPARE(state.reference().observation.position.coordinate().latitude(), 47.5);
    session.stop();
    QVERIFY(!state.reference().isValid());
}

void GPSBaseStationStateTest::_saveReference_data()
{
    QTest::addColumn<GPSBaseReference>("reference");
    QTest::addColumn<double>("savedAccuracy");
    QTest::addColumn<bool>("accepted");
    QTest::addColumn<double>("expectedAccuracy");
    QTest::addColumn<double>("expectedAltitude");
    GPSBaseReference reference;
    reference.valid = true;
    reference.observation.position = QGeoPositionInfo(QGeoCoordinate(47.5, 8.5, 500), QDateTime::currentDateTimeUtc());
    reference.observation.monotonicTimestampUs = 9000000;
    reference.observation.sessionId = 11;
    reference.observation.altitudeDatum = GPSObservation::AltitudeDatum::Ellipsoid;
    reference.accuracyMeters = 1.25;
    const auto row = [](const char* name, const GPSBaseReference& value, bool accepted,
                        double expectedAccuracy = 1.25, double expectedAltitude = 500) {
        QTest::newRow(name) << value << 4.5 << accepted << expectedAccuracy << expectedAltitude;
    };
    row("known", reference, true);
    auto changed = reference;
    changed.accuracyMeters.reset();
    row("unknown-preserves-configured", changed, true, 4.5);
    QTest::newRow("unknown-preserves-configured-zero") << changed << 0.0 << true << 0.0 << 500.0;
    QTest::newRow("unknown-with-invalid-configured") << changed << qQNaN() << false << 0.0 << 500.0;
    changed.accuracyMeters = 0;
    row("known-zero", changed, true, 0);
    changed.accuracyMeters = -1;
    row("negative-accuracy", changed, false);
    changed.accuracyMeters = qQNaN();
    row("nan-accuracy", changed, false);
    changed = reference;
    changed.observation.altitudeDatum = GPSObservation::AltitudeDatum::Unknown;
    row("unknown-datum", changed, false);
    changed.observation.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
    row("msl-without-ellipsoid", changed, false);
    changed.observation.altitudeEllipsoidMeters = 550;
    row("msl-with-known-ellipsoid", changed, true, 1.25, 550);
    changed = reference;
    changed.valid = false;
    row("invalid-reference", changed, false);
    changed = reference;
    changed.observation.sessionId = 10;
    row("retired-session", changed, false);
    changed = reference;
    changed.observation.monotonicTimestampUs = 0;
    row("missing-receipt", changed, false);
    changed.observation.monotonicTimestampUs = 10000001;
    row("future-receipt", changed, false);
    changed = reference;
    changed.observation.position.setCoordinate(QGeoCoordinate(91, 8.5, 500));
    row("invalid-coordinate", changed, false);
}

void GPSBaseStationStateTest::_saveReference()
{
    QFETCH(GPSBaseReference, reference);
    QFETCH(double, savedAccuracy);
    QFETCH(bool, accepted);
    QFETCH(double, expectedAccuracy);
    QFETCH(double, expectedAltitude);
    GPSReceiverConfig current;
    current.base.useFixedBase = true;
    current.base.fixedBaseAccuracyMeters = static_cast<float>(savedAccuracy);
    const auto result = GPSBaseReferenceSave::prepare(reference, 11, current, 10000000);
    QCOMPARE(result.configuration.has_value(), accepted);
    QCOMPARE(result.error.isEmpty(), accepted);
    QCOMPARE(current.base.fixedBaseLatitude, 0.0);
    if (accepted) {
        QCOMPARE(result.configuration->base.fixedBaseLatitude, 47.5);
        QCOMPARE(result.configuration->base.fixedBaseLongitude, 8.5);
        QCOMPARE(result.configuration->base.fixedBaseAltitudeMeters, static_cast<float>(expectedAltitude));
        QCOMPARE(result.configuration->base.fixedBaseAccuracyMeters, static_cast<float>(expectedAccuracy));
        QVERIFY(result.configuration->validationError().isEmpty());
    }
}

void GPSBaseStationStateTest::_saveSettingsAtomically()
{
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    RTKSettings settings;
    GPSReceiverConfig configuration;
    configuration.base.useFixedBase = true;
    configuration.base.fixedBaseLatitude = 47.5;
    configuration.base.fixedBaseLongitude = 8.5;
    configuration.base.fixedBaseAltitudeMeters = 500;
    configuration.base.fixedBaseAccuracyMeters = 0;
    settings.fixedBasePositionLatitude()->setRawValue(1.0);
    settings.fixedBasePositionLongitude()->setRawValue(2.0);
    settings.fixedBasePositionAltitude()->setRawValue(3.0);
    settings.fixedBasePositionAccuracy()->setRawValue(2.0);
    int notifications = 0;
    for (auto* fact : {settings.fixedBasePositionLatitude(), settings.fixedBasePositionLongitude(),
                       settings.fixedBasePositionAltitude(), settings.fixedBasePositionAccuracy()}) {
        connect(fact, &Fact::rawValueChanged, &settings, [&]() {
            ++notifications;
            QCOMPARE(settings.fixedBasePositionLatitude()->rawValue().toDouble(), 47.5);
            QCOMPARE(settings.fixedBasePositionLongitude()->rawValue().toDouble(), 8.5);
            QCOMPARE(settings.fixedBasePositionAltitude()->rawValue().toFloat(), 500.0f);
            QCOMPARE(settings.fixedBasePositionAccuracy()->rawValue().toFloat(), 0.0f);
            QSettings storage;
            QCOMPARE(storage.value(QStringLiteral("RTK/fixedBasePositionLatitude")).toDouble(), 47.5);
            QCOMPARE(storage.value(QStringLiteral("RTK/fixedBasePositionLongitude")).toDouble(), 8.5);
            QCOMPARE(storage.value(QStringLiteral("RTK/fixedBasePositionAltitude")).toFloat(), 500.0f);
            QCOMPARE(storage.value(QStringLiteral("RTK/fixedBasePositionAccuracy")).toFloat(), 0.0f);
        });
    }
    QVERIFY(settings.saveFixedBasePosition(configuration));
    QCOMPARE(notifications, 4);
    QVERIFY(settings.saveFixedBasePosition(configuration));
    QCOMPARE(notifications, 4);
    configuration.base.fixedBaseAccuracyMeters = qQNaN();
    QVERIFY(!settings.saveFixedBasePosition(configuration));
    QCOMPARE(notifications, 4);
    QCOMPARE(settings.fixedBasePositionAccuracy()->rawValue().toFloat(), 0.0f);
}

void GPSBaseStationStateTest::_saveSettingsReentrantEdit()
{
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    RTKSettings settings;
    settings.fixedBasePositionLatitude()->setRawValue(1.0);
    GPSReceiverConfig configuration;
    configuration.base.useFixedBase = true;
    configuration.base.fixedBaseLatitude = 47.5;
    configuration.base.fixedBaseLongitude = 8.5;
    configuration.base.fixedBaseAltitudeMeters = 500;
    configuration.base.fixedBaseAccuracyMeters = 0;
    connect(settings.fixedBasePositionLatitude(), &Fact::valueChanged, &settings, [&]() {
        QCOMPARE(settings.fixedBasePositionAccuracy()->rawValue().toDouble(), 0.0);
        settings.fixedBasePositionAccuracy()->setRawValue(2.5);
    });
    QVERIFY(settings.saveFixedBasePosition(configuration));
    QCOMPARE(settings.fixedBasePositionAccuracy()->rawValue().toDouble(), 2.5);
    QCOMPARE(QSettings().value(QStringLiteral("RTK/fixedBasePositionAccuracy")).toDouble(), 2.5);
}

void GPSBaseStationStateTest::_saveSettingsNotificationCanDestroySettings()
{
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    auto settings = std::make_unique<RTKSettings>();
    settings->fixedBasePositionLatitude()->setRawValue(1.0);
    GPSReceiverConfig configuration;
    configuration.base.useFixedBase = true;
    configuration.base.fixedBaseLatitude = 47.5;
    configuration.base.fixedBaseLongitude = 8.5;
    configuration.base.fixedBaseAltitudeMeters = 500;
    configuration.base.fixedBaseAccuracyMeters = 0;
    connect(settings->fixedBasePositionLatitude(), &Fact::valueChanged, this, [&]() { settings.reset(); });
    QVERIFY(!settings->saveFixedBasePosition(configuration));
    QVERIFY(!settings);
    QSettings storage;
    QCOMPARE(storage.value(QStringLiteral("RTK/fixedBasePositionLatitude")).toDouble(), 47.5);
    QCOMPARE(storage.value(QStringLiteral("RTK/fixedBasePositionLongitude")).toDouble(), 8.5);
    QCOMPARE(storage.value(QStringLiteral("RTK/fixedBasePositionAltitude")).toDouble(), 500.0);
    QCOMPARE(storage.value(QStringLiteral("RTK/fixedBasePositionAccuracy")).toDouble(), 0.0);
}

UT_REGISTER_TEST(GPSBaseStationStateTest, TestLabel::Unit)
