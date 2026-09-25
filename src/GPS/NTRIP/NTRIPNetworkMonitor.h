#pragma once

#include <QtCore/QObject>

class QNetworkInformation;

class NTRIPNetworkMonitor : public QObject
{
    Q_OBJECT

public:
    explicit NTRIPNetworkMonitor(QObject* parent = nullptr);

    virtual bool hasNetwork() const = 0;

signals:
    void networkChanged(bool available);
};

class QtNTRIPNetworkMonitor : public NTRIPNetworkMonitor
{
    Q_OBJECT

public:
    explicit QtNTRIPNetworkMonitor(QObject* parent = nullptr);

    bool hasNetwork() const override;

private:
    QNetworkInformation* _networkInformation = nullptr;
    bool _hasNetwork = true;
};
