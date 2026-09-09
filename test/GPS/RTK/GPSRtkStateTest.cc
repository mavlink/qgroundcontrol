#include "GPSRtkStateTest.h"

#include <QtCore/QPointer>

#include <memory>

#include "GPSReceiverSession.h"
#include "GPSRtkState.h"

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

void GPSRtkStateTest::_attachReceiver(GPSReceiverSession& session, GPSReceiverConfig::Role role,
                                      GPSReceiverCapabilities::Support support)
{
    session.stop();
    session._config.role = role;
    session._capabilities = GPSReceiverCapabilities::forType(GPSType::u_blox);
    session._capabilities.rtkBase = support;
    session._provider = new GPSProvider({}, GPSType::u_blox, session._config, {}, &session);
    emit session.receiverTypeChanged(GPSType::u_blox);
}

void GPSRtkStateTest::_surveyRoleGating_data()
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

void GPSRtkStateTest::_surveyRoleGating()
{
    QFETCH(bool, base);
    QFETCH(GPSReceiverCapabilities::Support, support);
    QFETCH(bool, hasReceiver);
    QFETCH(bool, accepted);
    GPSReceiverSession session;
    GPSRtkState state(session);
    _attachReceiver(session, base ? GPSReceiverConfig::Role::RTKBase : GPSReceiverConfig::Role::Position, support);
    if (!hasReceiver) {
        session.stop();
    }
    emit session.surveyInReceived(validSurvey());
    QCOMPARE(state.facts()->valid()->rawValue().toBool(), accepted);
    QCOMPARE(state.facts()->active()->rawValue().toBool(), accepted);
    QCOMPARE(state.facts()->currentDuration()->rawValue().toInt(), accepted ? 180 : 0);
    if (accepted) {
        QCOMPARE(state.facts()->currentAccuracy()->rawValue().toDouble(), 0.25);
        QCOMPARE(state.facts()->currentLatitude()->rawValue().toDouble(), 47.5);
        QCOMPARE(state.facts()->currentLongitude()->rawValue().toDouble(), 8.5);
        QCOMPARE(state.facts()->currentAltitude()->rawValue().toDouble(), 500.0);
    } else {
        QVERIFY(qIsNaN(state.facts()->currentLatitude()->rawValue().toDouble()));
    }
    QCOMPARE(state.facts()->factNames().size(), 7);
    QVERIFY(!state.facts()->factNames().contains(QStringLiteral("connected")));
    QVERIFY(!state.facts()->factNames().contains(QStringLiteral("lastError")));
    QVERIFY(!state.facts()->factNames().contains(QStringLiteral("numSatellites")));
}

void GPSRtkStateTest::_roleChangeAndDisconnectReset()
{
    GPSReceiverSession session;
    GPSRtkState state(session);
    _attachReceiver(session, GPSReceiverConfig::Role::RTKBase, GPSReceiverCapabilities::Support::Unknown);
    emit session.surveyInReceived(validSurvey());
    QVERIFY(state.facts()->valid()->rawValue().toBool());
    _attachReceiver(session, GPSReceiverConfig::Role::Position, GPSReceiverCapabilities::Support::Supported);
    emit session.surveyInReceived(validSurvey());
    QVERIFY(!state.facts()->valid()->rawValue().toBool());
    QVERIFY(!state.facts()->active()->rawValue().toBool());
    QVERIFY(qIsNaN(state.facts()->currentAccuracy()->rawValue().toDouble()));
    QVERIFY(qIsNaN(state.facts()->currentLatitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(state.facts()->currentLongitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(state.facts()->currentAltitude()->rawValue().toDouble()));

    _attachReceiver(session, GPSReceiverConfig::Role::RTKBase, GPSReceiverCapabilities::Support::Supported);
    emit session.surveyInReceived(validSurvey());
    session._capabilities.rtkBase = GPSReceiverCapabilities::Support::Unsupported;
    emit session.capabilitiesUpdated(session.capabilities());
    QVERIFY(!state.facts()->valid()->rawValue().toBool());
    emit session.surveyInReceived(validSurvey());
    QCOMPARE(state.facts()->currentDuration()->rawValue().toInt(), 0);
    session.stop();
    emit session.surveyInReceived(validSurvey());
    QVERIFY(!state.facts()->valid()->rawValue().toBool());
}

void GPSRtkStateTest::_roleChangeDuringSurveyUpdate()
{
    GPSReceiverSession session;
    GPSRtkState state(session);
    _attachReceiver(session, GPSReceiverConfig::Role::RTKBase, GPSReceiverCapabilities::Support::Supported);
    bool switched = false;
    connect(state.facts()->currentDuration(), &Fact::rawValueChanged, this, [&]() {
        if (!switched) {
            switched = true;
            _attachReceiver(session, GPSReceiverConfig::Role::Position, GPSReceiverCapabilities::Support::Supported);
        }
    });
    emit session.surveyInReceived(validSurvey());
    QVERIFY(switched);
    QCOMPARE(session.config().role, GPSReceiverConfig::Role::Position);
    QVERIFY(!state.facts()->valid()->rawValue().toBool());
    QVERIFY(!state.facts()->active()->rawValue().toBool());
    QCOMPARE(state.facts()->currentDuration()->rawValue().toInt(), 0);
    QVERIFY(qIsNaN(state.facts()->currentLatitude()->rawValue().toDouble()));
    QVERIFY(qIsNaN(state.facts()->currentAccuracy()->rawValue().toDouble()));
}

void GPSRtkStateTest::_presentationDestructionKeepsSession()
{
    GPSReceiverSession session;
    _attachReceiver(session, GPSReceiverConfig::Role::RTKBase, GPSReceiverCapabilities::Support::Supported);
    const QPointer<GPSProvider> worker = session._provider;
    auto state = std::make_unique<GPSRtkState>(session);
    connect(state->facts()->currentDuration(), &Fact::rawValueChanged, this, [&]() { state.reset(); });
    emit session.surveyInReceived(validSurvey());
    QVERIFY(!state);
    QVERIFY(worker);
    QVERIFY(session.hasReceiver());
    session.stop();
    QVERIFY(!worker);
}

UT_REGISTER_TEST(GPSRtkStateTest, TestLabel::Unit)
