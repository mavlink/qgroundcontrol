#include "UomRegistrationClient.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslConfiguration>

#include "UomRegistrationCrypto.h"
#include "UomRegistrationStore.h"

namespace {
constexpr qint64 MAX_RESPONSE = 65536;
using UomRegistration::Operation;
using UomRegistration::Result;

bool validToken(const QString& token)
{
    if (token.isEmpty() || token.size() > 8192) {
        return false;
    }
    for (QChar c : token) {
        if (c.isNull() || c.category() == QChar::Other_Control) {
            return false;
        }
    }
    return true;
}
}  // namespace

UomRegistrationClient::UomRegistrationClient(UomRegistrationStore* store, QObject* parent)
    : UomRegistrationClient(store, new QNetworkAccessManager, parent)
{
    _network->setParent(this);
}

UomRegistrationClient::UomRegistrationClient(UomRegistrationStore* store, QNetworkAccessManager* network,
                                             QObject* parent, std::function<qint64()> now)
    : QObject(parent)
    , _store(store)
    , _network(network)
    , _now(now ? std::move(now) : std::function<qint64()>(QDateTime::currentMSecsSinceEpoch))
{}

UomRegistrationClient::~UomRegistrationClient()
{
    cancel();
}

void UomRegistrationClient::_reject(quint64 requestId, const QString& detail, int retryAfter)
{
    const quint64 generation = _generation;
    QTimer::singleShot(0, this, [this, requestId, detail, retryAfter, generation]() {
        if (generation == _generation) {
            emit completed(requestId,
                           Result{.retryable = retryAfter > 0, .retryAfterSeconds = retryAfter, .detail = detail});
        }
    });
}

void UomRegistrationClient::send(quint64 requestId, const UomRegistration::Config& config, const QString& keyHex,
                                 Operation operation, const QString& serial, const QString& type, const QString& reason,
                                 const QString& token)
{
    if (_reply || !_network) {
        _reject(requestId, tr("Another UOM request is still pending"));
        return;
    }
    if (!config.valid() || serial != config.expectedSerial || !UomRegistration::validKey(keyHex)) {
        _reject(requestId, tr("Check the official UOM profile, aircraft identity and encryption key"));
        return;
    }
    if (!_store || !_store->ready()) {
        _reject(requestId, tr("UOM transaction storage is unavailable"));
        return;
    }
    QJsonObject payload;
    if (operation == Operation::Cancel) {
        if (!validToken(token) ||
            (type != QStringLiteral("0") && type != QStringLiteral("3") && type != QStringLiteral("4")) ||
            reason.trimmed().isEmpty() || reason.toUtf8().size() > 200) {
            _reject(requestId,
                    tr("A cancellation token, reason type and reason of at most 200 UTF-8 bytes are required"));
            return;
        }
        payload = {
            {QStringLiteral("TOKEN"), token}, {QStringLiteral("TYPE"), type}, {QStringLiteral("REASON"), reason}};
    } else {
        payload.insert(QStringLiteral("UPIC_MSN"), serial);
        if (operation == Operation::ReportActive || operation == Operation::ReportInactive) {
            payload.insert(QStringLiteral("STATE"),
                           operation == Operation::ReportActive ? QStringLiteral("1") : QStringLiteral("2"));
        }
    }
    const QByteArray plaintext = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    const QByteArray fingerprintInput =
        config.endpoint(operation).toEncoded() + '\0' + serial.toUtf8() + '\0' + plaintext;
    const QString fingerprint =
        QString::fromLatin1(QCryptographicHash::hash(fingerprintInput, QCryptographicHash::Sha256).toHex());
    const QByteArray throttleIdentity =
        QJsonDocument(QJsonArray{config.environment, config.producerId}).toJson(QJsonDocument::Compact);
    const QString throttleScope =
        QString::fromLatin1(QCryptographicHash::hash(throttleIdentity, QCryptographicHash::Sha256).toHex());
    const int retryAfter = _store->retryAfter(throttleScope, fingerprint, _now());
    if (!_store->error().isEmpty() || retryAfter > 0) {
        _reject(requestId, tr("Identical UOM requests must be at least one minute apart"), retryAfter);
        return;
    }
    const auto encrypted = UomRegistration::encrypt(plaintext, keyHex);
    if (!encrypted) {
        _reject(requestId, tr("UOM request encryption failed"));
        return;
    }
    if (!_store->recordAttempt(throttleScope, fingerprint, _now())) {
        _reject(requestId, tr("The UOM request could not be recorded safely"));
        return;
    }
    QNetworkRequest request(config.endpoint(operation));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json;charset=utf-8"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(15000);
    QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
    ssl.setProtocol(QSsl::TlsV1_2OrLater);
    ssl.setPeerVerifyMode(QSslSocket::VerifyPeer);
    request.setSslConfiguration(ssl);
    const QByteArray envelope = QJsonDocument(QJsonObject{{QStringLiteral("id"), config.producerId},
                                                          {QStringLiteral("body"), QString::fromLatin1(*encrypted)}})
                                    .toJson(QJsonDocument::Compact);
    const quint64 generation = ++_generation;
    QNetworkReply* reply = _network->post(request, envelope);
    _reply = reply;
    QTimer::singleShot(15000, reply, [reply]() {
        if (!reply->isFinished()) {
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::readyRead, this, [reply]() {
        if (reply->bytesAvailable() > MAX_RESPONSE) {
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, generation, requestId, operation]() {
        reply->deleteLater();
        if (generation != _generation) {
            return;
        }
        _reply.clear();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        Result result;
        if (reply->error() != QNetworkReply::NoError && (status == 0 || status == 200)) {
            result.retryable = operation != Operation::Cancel;
            result.uncertain = operation != Operation::Verify;
            result.detail = tr("UOM did not return a confirmed result; check the saved operation before continuing");
        } else {
            result = parseResponse(operation, status, reply->readAll());
        }
        emit completed(requestId, result);
    });
}

void UomRegistrationClient::cancel()
{
    ++_generation;
    if (_reply) {
        _reply->abort();
        _reply->deleteLater();
        _reply.clear();
    }
}

Result UomRegistrationClient::parseResponse(Operation operation, int httpStatus, const QByteArray& body)
{
    const bool mutation = operation != Operation::Verify;
    if (httpStatus != 200) {
        return {.retryable = (httpStatus == 429 || httpStatus >= 500) && operation != Operation::Cancel,
                .uncertain = mutation && (httpStatus == 0 || httpStatus >= 500),
                .retryAfterSeconds = httpStatus == 429 ? 60 : 0,
                .detail = tr("UOM returned HTTP status %1").arg(httpStatus)};
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(body, &error);
    if (body.size() > MAX_RESPONSE || error.error != QJsonParseError::NoError || !document.isObject() ||
        !document.object().value(QStringLiteral("code")).isDouble()) {
        return {.uncertain = mutation, .detail = tr("UOM returned an invalid response")};
    }
    const QJsonObject envelope = document.object();
    if (envelope.value(QStringLiteral("code")).toInt() != 200) {
        return {.detail = tr("UOM rejected the request (code %1)").arg(envelope.value(QStringLiteral("code")).toInt())};
    }
    const QJsonValue raw = envelope.value(QStringLiteral("body"));
    QJsonObject value;
    if (raw.isString()) {
        const QJsonDocument nested = QJsonDocument::fromJson(raw.toString().toUtf8(), &error);
        if (error.error == QJsonParseError::NoError && nested.isObject()) {
            value = nested.object();
        }
    } else if (raw.isObject()) {
        value = raw.toObject();
    }
    const QString state = value.value(QStringLiteral("value")).toString();
    const QString token = value.value(QStringLiteral("token")).toString();
    const bool valid =
        (operation == Operation::Verify &&
         (state == QStringLiteral("0") || state == QStringLiteral("3") || state == QStringLiteral("99"))) ||
        (operation == Operation::ReportActive && state == QStringLiteral("1")) ||
        (operation == Operation::ReportInactive && state == QStringLiteral("2") && validToken(token)) ||
        (operation == Operation::Cancel && state == QStringLiteral("3"));
    if (!valid) {
        return {.uncertain = mutation, .detail = tr("UOM returned a state that does not confirm this operation")};
    }
    return {.success = true,
            .value = state,
            .token = operation == Operation::ReportInactive ? token : QString(),
            .detail = tr("UOM confirmed the result")};
}
