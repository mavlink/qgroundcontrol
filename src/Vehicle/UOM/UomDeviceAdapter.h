#pragma once

#include <QtCore/QObject>
#include <QtCore/QStringList>

#include "UomRegistrationWorkflow.h"

/// Firmware-owned activation transport. Completion means durable device state was read back.
class UomDeviceAdapter : public QObject
{
    Q_OBJECT

public:
    explicit UomDeviceAdapter(QObject* parent = nullptr)
        : QObject(parent)
    {}

    virtual bool supported() const = 0;
    virtual bool busy() const = 0;
    virtual bool canWrite() const = 0;
    virtual QString productModel() const = 0;
    virtual QString statusText() const = 0;
    virtual QString safetyStatus() const = 0;
    virtual QStringList devices() const = 0;
    virtual int selectedDevice() const = 0;
    virtual UomRegistrationDevice device() const = 0;
    virtual void refresh() = 0;
    virtual void selectDevice(int index) = 0;
    virtual void readActivation(quint64 requestId) = 0;
    virtual void setActivation(quint64 requestId, const UomRegistrationDevice& expected, bool active) = 0;

signals:
    void changed();
    void activationRead(quint64 requestId, const UomRegistrationDevice& device);
    void activationWritten(quint64 requestId, const UomRegistrationDevice& device);
    void activationFailed(quint64 requestId, const QString& detail, bool uncertain);
};
