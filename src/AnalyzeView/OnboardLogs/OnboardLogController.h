#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "OnboardLogTransport.h"

class FtpTransport;
class LogProtocolTransport;
class QmlObjectListModel;
class Vehicle;

/// QML facade picking LOG-protocol vs MAVLink-FTP transport per active vehicle.
class OnboardLogController : public OnboardLogTransport
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_MOC_INCLUDE("Vehicle.h")

public:
    explicit OnboardLogController(QObject* parent = nullptr);
    ~OnboardLogController() override;

    QmlObjectListModel* model() const override;
    bool requestingList() const override;
    bool downloadingLogs() const override;
    bool canErase() const override;
    QString transportName() const override;

    Q_INVOKABLE void refresh() override;
    Q_INVOKABLE void download(const QString& path = QString()) override;
    Q_INVOKABLE void eraseAll() override;
    Q_INVOKABLE void cancel() override;

private slots:
    void _setActiveVehicle(Vehicle* vehicle);
    void _reevaluateTransport();

private:
    void _setActiveTransport(OnboardLogTransport* transport);

    LogProtocolTransport* _logTransport = nullptr;
    FtpTransport* _ftpTransport = nullptr;
    OnboardLogTransport* _active = nullptr;
    Vehicle* _vehicle = nullptr;
};
