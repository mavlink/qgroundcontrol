#include "GPSBaseStationStateTest.h"

#include <QtCore/QPointer>

#include <memory>

#include "GPSBaseStationState.h"
#include "GPSReceiverSession.h"

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
    GPSBaseStationFactGroup facts;
    GPSBaseStationState state(session, facts);
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
    GPSBaseStationFactGroup facts;
    GPSBaseStationState state(session, facts);
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
    GPSBaseStationFactGroup facts;
    GPSBaseStationState state(session, facts);
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
    _attachReceiver(session, GPSReceiverConfig::Role::RTKBase, GPSReceiverCapabilities::Support::Supported);
    const QPointer<GPSProvider> worker = session._provider;
    GPSBaseStationFactGroup facts;
    auto state = std::make_unique<GPSBaseStationState>(session, facts);
    connect(facts.currentDuration(), &Fact::rawValueChanged, this, [&]() { state.reset(); });
    emit session.surveyInReceived(validSurvey());
    QVERIFY(!state);
    QVERIFY(worker);
    QVERIFY(session.hasReceiver());
    session.stop();
    QVERIFY(!worker);
}

UT_REGISTER_TEST(GPSBaseStationStateTest, TestLabel::Unit)
