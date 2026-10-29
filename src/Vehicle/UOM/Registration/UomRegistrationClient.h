#pragma once

#include <functional>

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "UomRegistrationTypes.h"

class QNetworkAccessManager;
class QNetworkReply;
class UomRegistrationStore;

/// Encrypts official UOM requests and persists the duplicate-request cooldown before sending them.
class UomRegistrationClient final : public QObject
{
    Q_OBJECT
public:
    explicit UomRegistrationClient(UomRegistrationStore* store, QObject* parent = nullptr);
    UomRegistrationClient(UomRegistrationStore* store, QNetworkAccessManager* network, QObject* parent,
                          std::function<qint64()> now = {});
    ~UomRegistrationClient() override;

    void send(quint64 requestId, const UomRegistration::Config& config, const QString& keyHex,
              UomRegistration::Operation operation, const QString& serial, const QString& type = {},
              const QString& reason = {}, const QString& token = {});
    void cancel();
    [[nodiscard]] static UomRegistration::Result parseResponse(UomRegistration::Operation operation, int httpStatus,
                                                               const QByteArray& body);

signals:
    void completed(quint64 requestId, const UomRegistration::Result& result);

private:
    void _reject(quint64 requestId, const QString& detail, int retryAfter = 0);
    UomRegistrationStore* _store;
    QNetworkAccessManager* _network;
    QPointer<QNetworkReply> _reply;
    quint64 _generation = 0;
    std::function<qint64()> _now;
};
