#pragma once

#include "NTRIPTransportConfig.h"

#include <QtCore/QObject>
#include <QtNetwork/QNetworkReply>

class QNetworkAccessManager;

class NTRIPSourceTableFetcher : public QObject
{
    Q_OBJECT

public:
    static constexpr int kFetchTimeoutMs = 10000;

    explicit NTRIPSourceTableFetcher(const NTRIPTransportConfig& config,
                                     QObject* parent = nullptr);
    ~NTRIPSourceTableFetcher() override;

    void fetch();

signals:
    void sourceTableReceived(const QString& table);
    void error(const QString& errorMsg);
    void finished();

private:
    void _onReplyFinished();

    QNetworkAccessManager* _networkManager = nullptr;
    QNetworkReply* _reply = nullptr;
    NTRIPTransportConfig _config;
};
