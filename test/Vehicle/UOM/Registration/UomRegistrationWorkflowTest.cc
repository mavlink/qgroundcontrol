#include "UomRegistrationWorkflowTest.h"

#include <cstring>
#include <memory>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtTest/QTest>

#include "UnitTest.h"
#include "UomRegistrationClient.h"
#include "UomRegistrationCredentials.h"
#include "UomRegistrationCrypto.h"
#include "UomRegistrationStore.h"
#include "UomRegistrationWorkflow.h"

namespace {
const QString KEY = QStringLiteral("0123456789abcdeffedcba9876543210");
const QString SERIAL = QStringLiteral("TEST0000000000000001");
const QString UID = QStringLiteral("001122AABBCC");

QByteArray resultBody(const QString& value, const QString& token = {})
{
    QJsonObject body{{QStringLiteral("value"), value}};
    if (!token.isEmpty()) {
        body.insert(QStringLiteral("token"), token);
    }
    return QJsonDocument(QJsonObject{{QStringLiteral("code"), 200}, {QStringLiteral("body"), body}})
        .toJson(QJsonDocument::Compact);
}

struct ReplyPlan
{
    QByteArray body;
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    bool held = false;
    int status = 200;
};

class WorkflowReply final : public QNetworkReply
{
public:
    WorkflowReply(const QNetworkRequest& request, ReplyPlan plan, QObject* parent)
        : QNetworkReply(parent)
        , _plan(std::move(plan))
    {
        setRequest(request);
        setUrl(request.url());
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, _plan.status);
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        if (!_plan.held) {
            QTimer::singleShot(0, this, &WorkflowReply::finish);
        }
    }

    void finish()
    {
        if (_plan.error != QNetworkReply::NoError) {
            setError(_plan.error, QStringLiteral("Simulated network failure"));
        }
        setFinished(true);
        emit readyRead();
        emit finished();
    }

    void abort() override
    {
        if (!isFinished()) {
            setError(QNetworkReply::OperationCanceledError, QStringLiteral("Cancelled"));
            setFinished(true);
            emit finished();
        }
    }

    qint64 bytesAvailable() const override { return _plan.body.size() - _offset + QNetworkReply::bytesAvailable(); }

protected:
    qint64 readData(char* output, qint64 maximum) override
    {
        const qint64 count = qMin(maximum, _plan.body.size() - _offset);
        if (count <= 0) {
            return -1;
        }
        std::memcpy(output, _plan.body.constData() + _offset, size_t(count));
        _offset += count;
        return count;
    }

private:
    ReplyPlan _plan;
    qint64 _offset = 0;
};

struct CapturedRequest
{
    QString operation;
    QJsonObject payload;
    QPointer<WorkflowReply> reply;
    QString producerId;
    QUrl url;
};

class WorkflowNetwork final : public QNetworkAccessManager
{
public:
    QList<ReplyPlan> plans;
    QList<CapturedRequest> requests;
    QStringList* events = nullptr;
    QString error;
    QString key = KEY;

protected:
    QNetworkReply* createRequest(QNetworkAccessManager::Operation, const QNetworkRequest& request,
                                 QIODevice* outgoing) override
    {
        const auto envelope = QJsonDocument::fromJson(outgoing ? outgoing->readAll() : QByteArray()).object();
        const auto plaintext =
            UomRegistration::decrypt(envelope.value(QStringLiteral("body")).toString().toLatin1(), key);
        if (!plaintext) {
            error = QStringLiteral("Request did not contain valid encrypted JSON");
        }
        const auto payload = QJsonDocument::fromJson(plaintext.value_or(QByteArray())).object();
        QString operation;
        if (request.url().path().endsWith(QStringLiteral("verifyRegisterFactory"))) {
            operation = QStringLiteral("verify");
        } else if (request.url().path().endsWith(QStringLiteral("reportActiveStateFactory"))) {
            operation = QStringLiteral("report") + payload.value(QStringLiteral("STATE")).toString();
        } else if (request.url().path().endsWith(QStringLiteral("cancelRegisterFactory"))) {
            operation = QStringLiteral("cancel");
        } else {
            error = QStringLiteral("Unexpected official endpoint");
        }
        if (events) {
            events->append(operation);
        }
        ReplyPlan plan{.body = QByteArray(R"({"code":500})")};
        if (plans.isEmpty()) {
            error = QStringLiteral("Unexpected extra network request: ") + operation;
        } else {
            plan = plans.takeFirst();
        }
        auto* reply = new WorkflowReply(request, std::move(plan), this);
        requests.append({operation, payload, reply, envelope.value(QStringLiteral("id")).toString(), request.url()});
        return reply;
    }
};

UomRegistration::Config config()
{
    UomRegistration::Config value;
    value.producerId = QStringLiteral("12345678-1234-1234-1234-123456789012");
    value.expectedSerial = SERIAL;
    return value;
}

class WorkflowHarness
{
public:
    explicit WorkflowHarness(const QString& path, int initialState = 0, qint64 initialNow = 1000000)
        : store(path)
        , credentials(nullptr, true)
        , now(initialNow)
        , client(&store, &network, nullptr, [this]() { return now; })
        , workflow(&store, &client, &credentials)
    {
        device = {SERIAL, UID, 7, 12, 1, initialState};
        network.events = &events;
        QObject::connect(&workflow, &UomRegistrationWorkflow::readDeviceRequested, &workflow,
                         [this](quint64 requestId) {
                             events.append(QStringLiteral("read"));
                             QTimer::singleShot(0, &workflow, [this, requestId]() {
                                 ++device.generation;
                                 workflow.observeDevice(device, true);
                                 workflow.deviceRead(requestId, device);
                             });
                         });
        QObject::connect(&workflow, &UomRegistrationWorkflow::writeDeviceRequested, &workflow,
                         [this](quint64 requestId, const UomRegistrationDevice& expected, bool active) {
                             events.append(active ? QStringLiteral("write1") : QStringLiteral("write0"));
                             ++writes;
                             writeId = requestId;
                             writeActive = active;
                             if (expected.serial != device.serial || expected.uid != device.uid ||
                                 expected.boot != device.boot || expected.revision != device.revision ||
                                 expected.generation != device.generation) {
                                 error = QStringLiteral("Workflow attempted to write a stale device context");
                             }
                             if (completeWrites) {
                                 QTimer::singleShot(0, &workflow, [this]() { completeWrite(); });
                             }
                         });
    }

    bool start()
    {
        if (!store.ready() || !workflow.configure(config(), KEY)) {
            return false;
        }
        workflow.observeDevice(device, true);
        return true;
    }

    void completeWrite()
    {
        if (uncertainWrite) {
            workflow.deviceFailed(writeId, QStringLiteral("Saved result could not be confirmed"), true);
            return;
        }
        ++device.generation;
        ++device.revision;
        device.state = writeActive ? 1 : 0;
        workflow.observeDevice(device, true);
        workflow.deviceWritten(writeId, device);
    }

    UomRegistrationStore store;
    WorkflowNetwork network;
    UomRegistrationCredentials credentials;
    qint64 now;
    UomRegistrationClient client;
    UomRegistrationWorkflow workflow;
    UomRegistrationDevice device;
    QStringList events;
    QString error;
    int writes = 0;
    bool completeWrites = true;
    bool uncertainWrite = false;
    quint64 writeId = 0;
    bool writeActive = false;
};
}  // namespace

void UomRegistrationWorkflowTest::_explicitIdentityIsRequired()
{
    WorkflowHarness h(QStringLiteral(":memory:"));
    QVERIFY(h.workflow.config().expectedSerial.isEmpty());
    h.workflow.observeDevice(h.device, true);
    QVERIFY(!h.workflow.configured());
    QVERIFY(!h.workflow.canVerify());
    auto missingIdentity = config();
    missingIdentity.expectedSerial.clear();
    QVERIFY(!h.workflow.configure(missingIdentity, KEY));
    QVERIFY(h.network.requests.isEmpty());
    QCOMPARE(h.writes, 0);
}

void UomRegistrationWorkflowTest::_unknownActivationStateCannotWrite_data()
{
    QTest::addColumn<int>("state");
    QTest::newRow("unknown") << 2;
    QTest::newRow("negative") << -1;
    QTest::newRow("unsupported") << 3;
}

void UomRegistrationWorkflowTest::_unknownActivationStateCannotWrite()
{
    QFETCH(int, state);
    WorkflowHarness h(QStringLiteral(":memory:"), state);
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canVerify(), TestTimeout::mediumMs());
    QVERIFY(!h.workflow.canActivate());
    QVERIFY(!h.workflow.canDeactivate());
    h.workflow.verifyRegistration();
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
    QVERIFY(h.network.requests.isEmpty());
    QCOMPARE(h.writes, 0);
}

void UomRegistrationWorkflowTest::_activationOrderAndStateSeparation()
{
    QTemporaryDir directory;
    WorkflowHarness h(directory.filePath(QStringLiteral("registration.sqlite")));
    h.network.plans = {{resultBody(QStringLiteral("0"))}, {.body = resultBody(QStringLiteral("1")), .held = true}};
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canActivate(), TestTimeout::mediumMs());
    h.workflow.activate();
    QTRY_COMPARE_WITH_TIMEOUT(h.network.requests.size(), 2, TestTimeout::mediumMs());
    QCOMPARE(h.events, QStringList({"read", "verify", "write1", "report1"}));
    QCOMPARE(h.device.state, 1);
    QCOMPARE(h.workflow.registrationText(), QStringLiteral("Registered"));
    QCOMPARE(h.workflow.deviceActivationText(), QStringLiteral("Activated"));
    QVERIFY(h.workflow.pending());
    QVERIFY(h.workflow.lastReportedAt().isEmpty());
    QCOMPARE(h.workflow.reportText(), QStringLiteral("No report acknowledgement recorded"));
    h.network.requests.last().reply->finish();
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
    QVERIFY(!h.workflow.pending());
    QVERIFY(!h.workflow.lastReportedAt().isEmpty());
    QCOMPARE(h.writes, 1);
    QCOMPARE(h.network.error, QString());
    QCOMPARE(h.error, QString());
}

void UomRegistrationWorkflowTest::_alreadyActiveStillRequiresDurableWrite()
{
    QTemporaryDir directory;
    WorkflowHarness h(directory.filePath(QStringLiteral("registration.sqlite")), 1);
    h.network.plans = {{resultBody(QStringLiteral("0"))}, {resultBody(QStringLiteral("1"))}};
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canActivate(), TestTimeout::mediumMs());
    h.workflow.activate();
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
    QCOMPARE(h.events, QStringList({"read", "verify", "write1", "report1"}));
    QCOMPARE(h.writes, 1);
    QVERIFY(!h.workflow.pending());
    QCOMPARE(h.network.error, QString());
    QCOMPARE(h.error, QString());
}

void UomRegistrationWorkflowTest::_unregisteredAircraftCannotActivate_data()
{
    QTest::addColumn<QString>("registration");
    QTest::newRow("not-registered") << QStringLiteral("99");
    QTest::newRow("cancelled") << QStringLiteral("3");
}

void UomRegistrationWorkflowTest::_unregisteredAircraftCannotActivate()
{
    QFETCH(QString, registration);
    QTemporaryDir directory;
    WorkflowHarness h(directory.filePath(QStringLiteral("registration.sqlite")));
    h.network.plans = {{resultBody(registration)}};
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canActivate(), TestTimeout::mediumMs());
    h.workflow.activate();
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
    QCOMPARE(h.events, QStringList({"read", "verify"}));
    QCOMPARE(h.writes, 0);
    QCOMPARE(h.device.state, 0);
    QVERIFY(!h.workflow.pending());
    QVERIFY(!h.store.loadPendingForDevice(SERIAL, UID));
    QCOMPARE(h.network.error, QString());
}

void UomRegistrationWorkflowTest::_initialBusinessRejectionDoesNotLockConfiguration()
{
    QTemporaryDir directory;
    WorkflowHarness h(directory.filePath(QStringLiteral("registration.sqlite")));
    h.network.plans = {{QByteArray(R"({"code":401,"msg":"invalid producer"})")}};
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canActivate(), TestTimeout::mediumMs());
    h.workflow.activate();
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
    QCOMPARE(h.writes, 0);
    QVERIFY(!h.workflow.pending());
    auto replacement = config();
    replacement.producerId = QStringLiteral("87654321-4321-4321-4321-210987654321");
    QVERIFY(h.workflow.configure(replacement, KEY));
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.configured(), TestTimeout::mediumMs());
    QCOMPARE(h.workflow.config().producerId, replacement.producerId);
}

void UomRegistrationWorkflowTest::_uncertainDeviceWriteDoesNotReport()
{
    QTemporaryDir directory;
    WorkflowHarness h(directory.filePath(QStringLiteral("registration.sqlite")));
    h.uncertainWrite = true;
    h.network.plans = {{resultBody(QStringLiteral("0"))}};
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canActivate(), TestTimeout::mediumMs());
    h.workflow.activate();
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
    QCOMPARE(h.events, QStringList({"read", "verify", "write1"}));
    QVERIFY(h.workflow.pending());
    QVERIFY(h.workflow.lastReportedAt().isEmpty());
    const auto saved = h.store.loadPendingForDevice(SERIAL, UID);
    QVERIFY(saved);
    QCOMPARE(saved->stage, QStringLiteral("writing"));
    QCOMPARE(h.network.error, QString());
}

void UomRegistrationWorkflowTest::_activationReportRecoveryDoesNotRewriteDevice()
{
    QTemporaryDir directory;
    WorkflowHarness h(directory.filePath(QStringLiteral("registration.sqlite")));
    h.network.plans = {
        {resultBody(QStringLiteral("0"))}, {.body = "unavailable", .status = 503}, {resultBody(QStringLiteral("1"))}};
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canActivate(), TestTimeout::mediumMs());
    h.workflow.activate();
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
    QCOMPARE(h.device.state, 1);
    QVERIFY(h.workflow.pending());
    QVERIFY(h.workflow.lastReportedAt().isEmpty());
    QCOMPARE(h.writes, 1);
    h.now += 60001;
    QVERIFY(h.workflow.canRetry());
    h.workflow.retryPending();
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.pending(), TestTimeout::mediumMs());
    QCOMPARE(h.events, QStringList({"read", "verify", "write1", "report1", "read", "report1"}));
    QCOMPARE(h.writes, 1);
    QCOMPARE(h.network.error, QString());
}

void UomRegistrationWorkflowTest::_automaticRetryRequiresDurableProof_data()
{
    QTest::addColumn<QString>("mismatch");
    for (const char* mismatch : {"missing", "boot", "revision", "state"}) {
        QTest::newRow(mismatch) << QString::fromLatin1(mismatch);
    }
}

void UomRegistrationWorkflowTest::_automaticRetryRequiresDurableProof()
{
    QFETCH(QString, mismatch);
    QTemporaryDir directory;
    WorkflowHarness h(directory.filePath(QStringLiteral("registration.sqlite")));
    if (mismatch != QStringLiteral("missing")) {
        h.network.plans.append({resultBody(QStringLiteral("0"))});
    }
    h.network.plans.append({.body = "unavailable", .status = 503});
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canActivate(), TestTimeout::mediumMs());
    h.workflow.activate();
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
    QVERIFY(h.workflow.pending());
    QVERIFY(h.workflow._retry.isActive());
    if (mismatch == QStringLiteral("boot")) {
        ++h.device.boot;
    } else if (mismatch == QStringLiteral("revision")) {
        ++h.device.revision;
    } else if (mismatch == QStringLiteral("state")) {
        h.device.state = 0;
    }
    h.workflow.observeDevice(h.device, true);
    const auto requests = h.network.requests.size();
    const int writes = h.writes;
    h.events.clear();
    h.now += 60001;
    QVERIFY(QMetaObject::invokeMethod(&h.workflow._retry, "timeout", Qt::DirectConnection));
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
    QCOMPARE(h.events, QStringList({"read"}));
    QCOMPARE(h.network.requests.size(), requests);
    QCOMPARE(h.writes, writes);
    QVERIFY(h.workflow.pending());
    QVERIFY(!h.workflow._retry.isActive());
    QVERIFY(h.workflow.lastReportedAt().isEmpty());
    QCOMPARE(h.network.error, QString());
}

void UomRegistrationWorkflowTest::_pendingCancellationCanRepairProducerProfile()
{
    QTemporaryDir directory;
    WorkflowHarness h(directory.filePath(QStringLiteral("registration.sqlite")), 1);
    h.network.plans = {{.body = R"({"code":401})"}, {.body = R"({"code":401})"}};
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canDeactivate(), TestTimeout::mediumMs());
    h.workflow.deactivateAndCancel(QStringLiteral("3"), QStringLiteral("Ownership change"));
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
    QCOMPARE(h.events, QStringList({"read", "write0", "report2", "verify"}));
    const auto previous = h.store.loadPendingForDevice(SERIAL, UID);
    QVERIFY(previous);
    QVERIFY(previous->deviceVerified);
    auto repaired = config();
    repaired.environment = QStringLiteral("official");
    QVERIFY(!h.workflow.configure(repaired, KEY));
    repaired = config();
    repaired.expectedSerial[0] = u'X';
    QVERIFY(!h.workflow.configure(repaired, KEY));
    repaired = config();
    repaired.producerId = QStringLiteral("22345678-1234-1234-1234-123456789012");
    h.network.key = QStringLiteral("fedcba98765432100123456789abcdef");
    QVERIFY(h.workflow.configure(repaired, h.network.key));
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canRetry(), TestTimeout::mediumMs());
    const auto migrated = h.store.loadPendingForDevice(SERIAL, UID);
    QVERIFY(migrated);
    QCOMPARE(migrated->id, previous->id);
    QCOMPARE(migrated->scope, repaired.scope());
    QCOMPARE(migrated->stage, QStringLiteral("profile-recovery"));
    QCOMPARE(migrated->reason, QStringLiteral("Ownership change"));
    QCOMPARE(migrated->uid, UID);
    QVERIFY(migrated->deviceVerified);
    h.network.plans = {{resultBody(QStringLiteral("0"))},
                       {resultBody(QStringLiteral("2"), QStringLiteral("repaired-producer-token"))},
                       {resultBody(QStringLiteral("3"))}};
    h.events.clear();
    h.now += 60001;
    h.workflow.retryPending();
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.pending(), TestTimeout::mediumMs());
    QCOMPARE(h.events, QStringList({"read", "verify", "report2", "cancel"}));
    QCOMPARE(h.writes, 1);
    const auto& cancelled = h.network.requests.last();
    QCOMPARE(cancelled.producerId, repaired.producerId);
    QCOMPARE(cancelled.payload.value(QStringLiteral("TOKEN")).toString(), QStringLiteral("repaired-producer-token"));
    QCOMPARE(cancelled.payload.value(QStringLiteral("TYPE")).toString(), QStringLiteral("3"));
    QCOMPARE(cancelled.payload.value(QStringLiteral("REASON")).toString(), QStringLiteral("Ownership change"));
    QCOMPARE(h.network.error, QString());
    QCOMPARE(h.error, QString());
}

void UomRegistrationWorkflowTest::_pendingCancellationCanRepairCancelUrl_data()
{
    QTest::addColumn<bool>("timedOut");
    QTest::newRow("timeout-reconciled") << true;
    QTest::newRow("wrong-route-rejected") << false;
}

void UomRegistrationWorkflowTest::_pendingCancellationCanRepairCancelUrl()
{
    QFETCH(bool, timedOut);
    QTemporaryDir directory;
    WorkflowHarness h(directory.filePath(QStringLiteral("registration.sqlite")), 1);
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canDeactivate(), TestTimeout::mediumMs());
    auto wrong = config();
    wrong.cancelUrl.setPath(QStringLiteral("/wrong/cancelRegisterFactory"));
    QVERIFY(h.workflow.configure(wrong, KEY));
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canDeactivate(), TestTimeout::mediumMs());
    h.network.plans = {{resultBody(QStringLiteral("2"), QStringLiteral("old-route-token"))}};
    if (timedOut) {
        h.network.plans.append({.body = {}, .error = QNetworkReply::TimeoutError});
        h.network.plans.append({resultBody(QStringLiteral("0"))});
    } else {
        h.network.plans.append({.body = "route not found", .status = 404});
    }
    h.workflow.deactivateAndCancel(QStringLiteral("3"), QStringLiteral("Ownership change"));
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
    const auto previous = h.store.loadPendingForDevice(SERIAL, UID);
    QVERIFY(previous);
    QVERIFY(!previous->tokenReference.isEmpty());
    QCOMPARE(h.events.count(QStringLiteral("cancel")), 1);
    QCOMPARE(h.events.count(QStringLiteral("verify")), timedOut ? 1 : 0);
    QCOMPARE(h.network.requests.at(1).url, wrong.cancelUrl);
    QVERIFY(h.workflow.configure(config(), KEY));
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canRetry(), TestTimeout::mediumMs());
    const auto migrated = h.store.loadPendingForDevice(SERIAL, UID);
    QVERIFY(migrated);
    QCOMPARE(migrated->id, previous->id);
    QCOMPARE(migrated->stage, QStringLiteral("profile-recovery"));
    QVERIFY(migrated->tokenReference.isEmpty());
    QVERIFY(migrated->deviceVerified);
    h.network.plans = {{resultBody(QStringLiteral("0"))},
                       {resultBody(QStringLiteral("2"), QStringLiteral("new-route-token"))},
                       {resultBody(QStringLiteral("3"))}};
    h.events.clear();
    h.now += 60001;
    h.workflow.retryPending();
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.pending(), TestTimeout::mediumMs());
    QCOMPARE(h.events, QStringList({"read", "verify", "report2", "cancel"}));
    QCOMPARE(h.writes, 1);
    const auto& cancelled = h.network.requests.last();
    QCOMPARE(cancelled.payload.value(QStringLiteral("TOKEN")).toString(), QStringLiteral("new-route-token"));
    QCOMPARE(cancelled.payload.value(QStringLiteral("REASON")).toString(), QStringLiteral("Ownership change"));
    QCOMPARE(cancelled.url, config().cancelUrl);
    QCOMPARE(h.network.error, QString());
    QCOMPARE(h.error, QString());
}

void UomRegistrationWorkflowTest::_cancellationOrder()
{
    QTemporaryDir directory;
    WorkflowHarness h(directory.filePath(QStringLiteral("registration.sqlite")), 1);
    h.network.plans = {{resultBody(QStringLiteral("2"), QStringLiteral("cancellation-token"))},
                       {resultBody(QStringLiteral("3"))}};
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canDeactivate(), TestTimeout::mediumMs());
    h.workflow.deactivateAndCancel(QStringLiteral("3"), QStringLiteral("Ownership change"));
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
    QCOMPARE(h.events, QStringList({"read", "write0", "report2", "cancel"}));
    QCOMPARE(h.device.state, 0);
    QVERIFY(!h.workflow.pending());
    QCOMPARE(h.workflow.registrationText(), QStringLiteral("Registration cancelled"));
    QCOMPARE(h.network.requests.at(0).payload.value(QStringLiteral("STATE")).toString(), QStringLiteral("2"));
    const auto cancelled = h.network.requests.at(1).payload;
    QCOMPARE(cancelled.value(QStringLiteral("TOKEN")).toString(), QStringLiteral("cancellation-token"));
    QCOMPARE(cancelled.value(QStringLiteral("TYPE")).toString(), QStringLiteral("3"));
    QCOMPARE(cancelled.value(QStringLiteral("REASON")).toString(), QStringLiteral("Ownership change"));
    QVERIFY(!cancelled.contains(QStringLiteral("UPIC_MSN")));
    QCOMPARE(h.network.error, QString());
    QCOMPARE(h.error, QString());
}

void UomRegistrationWorkflowTest::_inactiveReportRejectionChecksExistingDeregistration()
{
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("registration.sqlite"));
    {
        WorkflowHarness h(path, 1);
        h.network.plans = {{QByteArray(R"({"code":409,"msg":"registration already cancelled"})")},
                           {resultBody(QStringLiteral("3"))}};
        QVERIFY(h.start());
        QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canDeactivate(), TestTimeout::mediumMs());
        h.workflow.deactivateAndCancel(QStringLiteral("3"), QStringLiteral("Ownership change"));
        QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
        QCOMPARE(h.events, QStringList({"read", "write0", "report2", "verify"}));
        QCOMPARE(h.device.state, 0);
        QVERIFY(!h.workflow.pending());
        QCOMPARE(h.workflow.registrationText(), QStringLiteral("Registration cancelled"));
        QCOMPARE(h.events.count(QStringLiteral("cancel")), 0);
        QVERIFY(h.workflow.lastReportedAt().isEmpty());
        QCOMPARE(h.workflow.reportText(), QStringLiteral("No report acknowledgement recorded"));
        QVERIFY(!h.store.loadLatestReport(config().scope(), SERIAL, UID));
        QCOMPARE(h.network.error, QString());
    }
    WorkflowHarness recovered(path, 0);
    QVERIFY(recovered.start());
    QTRY_VERIFY_WITH_TIMEOUT(recovered.workflow.configured(), TestTimeout::mediumMs());
    QVERIFY(!recovered.workflow.pending());
    QVERIFY(recovered.workflow.lastReportedAt().isEmpty());
    QCOMPARE(recovered.workflow.reportText(), QStringLiteral("No report acknowledgement recorded"));
}

void UomRegistrationWorkflowTest::_reportAcknowledgementSurvivesCompletionAndRestart()
{
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("registration.sqlite"));
    QString reportedAt;
    {
        WorkflowHarness h(path, 1);
        h.network.plans = {{resultBody(QStringLiteral("2"), QStringLiteral("cancellation-token"))},
                           {.body = resultBody(QStringLiteral("3")), .held = true}};
        QVERIFY(h.start());
        QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canDeactivate(), TestTimeout::mediumMs());
        h.workflow.deactivateAndCancel(QStringLiteral("3"), QStringLiteral("Ownership change"));
        QTRY_COMPARE_WITH_TIMEOUT(h.network.requests.size(), 2, TestTimeout::mediumMs());
        reportedAt = h.workflow.lastReportedAt();
        QVERIFY(!reportedAt.isEmpty());
        const auto report = h.store.loadLatestReport(config().scope(), SERIAL, UID);
        QVERIFY(report);
        QCOMPARE(report->reportedValue, QStringLiteral("2"));
        QCOMPARE(QDateTime::fromMSecsSinceEpoch(report->reportedAtMs).toString(Qt::ISODate), reportedAt);
        QCOMPARE(h.workflow.reportText(), QStringLiteral("Unactivated state report accepted"));
        h.network.requests.last().reply->finish();
        QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.pending(), TestTimeout::mediumMs());
        QCOMPARE(h.workflow.lastReportedAt(), reportedAt);
        // Deliberately distinguish transaction completion time from the earlier acknowledgement.
        auto completed = h.store.loadLatest(config().scope(), SERIAL, UID);
        QVERIFY(completed);
        completed->updatedMs = completed->reportedAtMs + 60000;
        QVERIFY(h.store.save(*completed));
        QCOMPARE(h.network.error, QString());
    }
    WorkflowHarness recovered(path, 0);
    QVERIFY(recovered.start());
    QTRY_VERIFY_WITH_TIMEOUT(recovered.workflow.configured(), TestTimeout::mediumMs());
    QCOMPARE(recovered.workflow.lastReportedAt(), reportedAt);
    QCOMPARE(recovered.workflow.reportText(), QStringLiteral("Unactivated state report accepted"));
    QVERIFY(recovered.events.isEmpty());
}

void UomRegistrationWorkflowTest::_reconciliationPreservesPreviousReportAcknowledgement()
{
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("registration.sqlite"));
    QString reportedAt;
    {
        WorkflowHarness h(path);
        h.network.plans = {{resultBody(QStringLiteral("0"))},
                           {resultBody(QStringLiteral("1"))},
                           {QByteArray(R"({"code":409,"msg":"registration already cancelled"})")},
                           {resultBody(QStringLiteral("3"))}};
        QVERIFY(h.start());
        QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canActivate(), TestTimeout::mediumMs());
        h.workflow.activate();
        QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
        reportedAt = h.workflow.lastReportedAt();
        QVERIFY(!reportedAt.isEmpty());
        h.now += 60001;
        h.workflow.deactivateAndCancel(QStringLiteral("3"), QStringLiteral("Ownership change"));
        QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
        QVERIFY(!h.workflow.pending());
        QCOMPARE(h.workflow.registrationText(), QStringLiteral("Registration cancelled"));
        QCOMPARE(h.workflow.reportText(), QStringLiteral("Activation report accepted"));
        QCOMPARE(h.workflow.lastReportedAt(), reportedAt);
        QCOMPARE(h.network.error, QString());
    }
    WorkflowHarness recovered(path, 0);
    QVERIFY(recovered.start());
    QTRY_VERIFY_WITH_TIMEOUT(recovered.workflow.configured(), TestTimeout::mediumMs());
    QCOMPARE(recovered.workflow.lastReportedAt(), reportedAt);
    QCOMPARE(recovered.workflow.reportText(), QStringLiteral("Activation report accepted"));
}

void UomRegistrationWorkflowTest::_cancellationTimeoutIsReconciled_data()
{
    QTest::addColumn<QString>("registration");
    QTest::addColumn<bool>("pending");
    QTest::newRow("already-cancelled") << QStringLiteral("3") << false;
    QTest::newRow("still-registered-needs-confirmation") << QStringLiteral("0") << true;
}

void UomRegistrationWorkflowTest::_cancellationTimeoutIsReconciled()
{
    QFETCH(QString, registration);
    QFETCH(bool, pending);
    QTemporaryDir directory;
    WorkflowHarness h(directory.filePath(QStringLiteral("registration.sqlite")), 1);
    h.network.plans = {{resultBody(QStringLiteral("2"), QStringLiteral("cancellation-token"))},
                       {.body = {}, .error = QNetworkReply::TimeoutError},
                       {resultBody(registration)}};
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canDeactivate(), TestTimeout::mediumMs());
    h.workflow.deactivateAndCancel(QStringLiteral("3"), QStringLiteral("Ownership change"));
    QTRY_VERIFY_WITH_TIMEOUT(!h.workflow.busy(), TestTimeout::mediumMs());
    QCOMPARE(h.events, QStringList({"read", "write0", "report2", "cancel", "verify"}));
    QCOMPARE(h.workflow.pending(), pending);
    QCOMPARE(h.device.state, 0);
    QCOMPARE(h.events.count(QStringLiteral("cancel")), 1);
    QCOMPARE(h.network.error, QString());
}

void UomRegistrationWorkflowTest::_restartWithoutProofRequiresExplicitWrite()
{
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("registration.sqlite"));
    {
        WorkflowHarness first(path);
        first.completeWrites = false;
        first.network.plans = {{resultBody(QStringLiteral("0"))}};
        QVERIFY(first.start());
        QTRY_VERIFY_WITH_TIMEOUT(first.workflow.canActivate(), TestTimeout::mediumMs());
        first.workflow.activate();
        QTRY_COMPARE_WITH_TIMEOUT(first.writes, 1, TestTimeout::mediumMs());
        QVERIFY(first.workflow.pending());
        const auto saved = first.store.loadPendingForDevice(SERIAL, UID);
        QVERIFY(saved);
        QCOMPARE(saved->stage, QStringLiteral("writing"));
    }
    WorkflowHarness recovered(path, 1, 1060001);
    recovered.network.plans = {{resultBody(QStringLiteral("0"))}, {resultBody(QStringLiteral("1"))}};
    QVERIFY(recovered.start());
    QTRY_VERIFY_WITH_TIMEOUT(recovered.workflow.configured(), TestTimeout::mediumMs());
    QVERIFY(recovered.workflow.pending());
    QVERIFY(recovered.events.isEmpty());
    QCOMPARE(recovered.writes, 0);
    recovered.workflow.retryPending();
    QTRY_VERIFY_WITH_TIMEOUT(!recovered.workflow.pending(), TestTimeout::mediumMs());
    QCOMPARE(recovered.events, QStringList({"read", "verify", "write1", "report1"}));
    QCOMPARE(recovered.writes, 1);
    QCOMPARE(recovered.network.error, QString());
}

void UomRegistrationWorkflowTest::_confirmedReportingRecovery_data()
{
    QTest::addColumn<bool>("sameBoot");
    QTest::newRow("matching-proof-does-not-rewrite") << true;
    QTest::newRow("restarted-device-requires-new-proof") << false;
}

void UomRegistrationWorkflowTest::_confirmedReportingRecovery()
{
    QFETCH(bool, sameBoot);
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("registration.sqlite"));
    UomRegistrationDevice verifiedDevice;
    {
        WorkflowHarness first(path);
        first.network.plans = {{resultBody(QStringLiteral("0"))},
                               {.body = resultBody(QStringLiteral("1")), .held = true}};
        QVERIFY(first.start());
        QTRY_VERIFY_WITH_TIMEOUT(first.workflow.canActivate(), TestTimeout::mediumMs());
        first.workflow.activate();
        QTRY_COMPARE_WITH_TIMEOUT(first.network.requests.size(), 2, TestTimeout::mediumMs());
        const auto saved = first.store.loadPendingForDevice(SERIAL, UID);
        QVERIFY(saved);
        QCOMPARE(saved->stage, QStringLiteral("reporting"));
        verifiedDevice = first.device;
        QCOMPARE(first.writes, 1);
    }
    WorkflowHarness recovered(path, 1, 1060001);
    recovered.device = verifiedDevice;
    ++recovered.device.generation;
    if (sameBoot) {
        recovered.network.plans = {{resultBody(QStringLiteral("1"))}};
    } else {
        ++recovered.device.boot;
        recovered.network.plans = {{resultBody(QStringLiteral("0"))}, {resultBody(QStringLiteral("1"))}};
    }
    QVERIFY(recovered.start());
    QTRY_VERIFY_WITH_TIMEOUT(recovered.workflow.configured(), TestTimeout::mediumMs());
    QVERIFY(recovered.workflow.pending());
    QVERIFY(recovered.events.isEmpty());
    QCOMPARE(recovered.writes, 0);
    recovered.workflow.retryPending();
    QTRY_VERIFY_WITH_TIMEOUT(!recovered.workflow.pending(), TestTimeout::mediumMs());
    QCOMPARE(recovered.events,
             sameBoot ? QStringList({"read", "report1"}) : QStringList({"read", "verify", "write1", "report1"}));
    QCOMPARE(recovered.writes, sameBoot ? 0 : 1);
    QCOMPARE(recovered.network.error, QString());
    QCOMPARE(recovered.error, QString());
}

void UomRegistrationWorkflowTest::_lostMemoryTokenRequiresExplicitRecovery()
{
    QTemporaryDir directory;
    const QString path = directory.filePath(QStringLiteral("registration.sqlite"));
    UomRegistrationDevice verifiedDevice;
    {
        WorkflowHarness first(path, 1);
        first.network.plans = {{resultBody(QStringLiteral("2"), QStringLiteral("old-memory-token"))},
                               {.body = resultBody(QStringLiteral("3")), .held = true}};
        QVERIFY(first.start());
        QTRY_VERIFY_WITH_TIMEOUT(first.workflow.canDeactivate(), TestTimeout::mediumMs());
        first.workflow.deactivateAndCancel(QStringLiteral("3"), QStringLiteral("Ownership change"));
        QTRY_COMPARE_WITH_TIMEOUT(first.network.requests.size(), 2, TestTimeout::mediumMs());
        QVERIFY(first.workflow.pending());
        const auto saved = first.store.loadPendingForDevice(SERIAL, UID);
        QVERIFY(saved);
        QVERIFY(!saved->tokenReference.isEmpty());
        verifiedDevice = first.device;
    }
    WorkflowHarness recovered(path, 0, 1060001);
    recovered.device = verifiedDevice;
    ++recovered.device.generation;
    recovered.network.plans = {{resultBody(QStringLiteral("0"))},
                               {resultBody(QStringLiteral("2"), QStringLiteral("new-memory-token"))},
                               {resultBody(QStringLiteral("3"))}};
    QVERIFY(recovered.start());
    QTRY_VERIFY_WITH_TIMEOUT(recovered.workflow.configured(), TestTimeout::mediumMs());
    QVERIFY(recovered.workflow.pending());
    QVERIFY(recovered.events.isEmpty());
    QCOMPARE(recovered.writes, 0);
    recovered.workflow.retryPending();
    QTRY_VERIFY_WITH_TIMEOUT(!recovered.workflow.pending(), TestTimeout::mediumMs());
    QCOMPARE(recovered.events, QStringList({"read", "verify", "report2", "cancel"}));
    QCOMPARE(recovered.writes, 0);
    QCOMPARE(recovered.network.requests.last().payload.value(QStringLiteral("TOKEN")).toString(),
             QStringLiteral("new-memory-token"));
    QCOMPARE(recovered.network.error, QString());
    QFile database(path);
    QVERIFY(database.open(QIODevice::ReadOnly));
    const auto stored = database.readAll();
    QVERIFY(!stored.contains("old-memory-token"));
    QVERIFY(!stored.contains("new-memory-token"));
    QVERIFY(!stored.contains(KEY.toLatin1()));
}

void UomRegistrationWorkflowTest::_lateVerificationCannotCrossDeviceContext_data()
{
    QTest::addColumn<QString>("field");
    for (const char* field : {"serial", "uid", "boot", "revision", "generation"}) {
        QTest::newRow(field) << QString::fromLatin1(field);
    }
}

void UomRegistrationWorkflowTest::_lateVerificationCannotCrossDeviceContext()
{
    QFETCH(QString, field);
    QTemporaryDir directory;
    WorkflowHarness h(directory.filePath(QStringLiteral("registration.sqlite")));
    h.network.plans = {{.body = resultBody(QStringLiteral("0")), .held = true}};
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canActivate(), TestTimeout::mediumMs());
    h.workflow.activate();
    QTRY_COMPARE_WITH_TIMEOUT(h.network.requests.size(), 1, TestTimeout::mediumMs());
    const auto oldReply = h.network.requests.first().reply;
    QVERIFY(oldReply);
    if (field == "serial") {
        h.device.serial[0] = u'X';
    } else if (field == "uid") {
        h.device.uid[0] = u'F';
    } else if (field == "boot") {
        ++h.device.boot;
    } else if (field == "revision") {
        ++h.device.revision;
    } else {
        ++h.device.generation;
    }
    h.workflow.observeDevice(h.device, true);
    QVERIFY(!h.workflow.busy());
    oldReply->finish();
    QCoreApplication::processEvents();
    QCOMPARE(h.writes, 0);
    QCOMPARE(h.network.requests.size(), 1);
    QCOMPARE(h.events, QStringList({"read", "verify"}));
    QCOMPARE(h.network.error, QString());
}

void UomRegistrationWorkflowTest::_lateDeviceWriteCannotCrossAircraft()
{
    QTemporaryDir directory;
    WorkflowHarness h(directory.filePath(QStringLiteral("registration.sqlite")));
    h.completeWrites = false;
    h.network.plans = {{resultBody(QStringLiteral("0"))}};
    QVERIFY(h.start());
    QTRY_VERIFY_WITH_TIMEOUT(h.workflow.canActivate(), TestTimeout::mediumMs());
    h.workflow.activate();
    QTRY_COMPARE_WITH_TIMEOUT(h.writes, 1, TestTimeout::mediumMs());
    auto oldDevice = h.device;
    oldDevice.state = 1;
    ++oldDevice.revision;
    ++oldDevice.generation;
    h.device.uid[0] = u'F';
    h.workflow.observeDevice(h.device, true);
    h.workflow.deviceWritten(h.writeId, oldDevice);
    QCoreApplication::processEvents();
    QVERIFY(!h.workflow.busy());
    QCOMPARE(h.network.requests.size(), 1);
    QVERIFY(h.workflow.lastReportedAt().isEmpty());
    QCOMPARE(h.network.error, QString());
}

QGC_REGISTER_PORTABLE_TEST(UomRegistrationWorkflowTest)
