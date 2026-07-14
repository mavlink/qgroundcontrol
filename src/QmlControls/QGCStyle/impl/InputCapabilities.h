#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class InputCapabilities : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool touchDeviceAvailable READ touchDeviceAvailable NOTIFY touchDeviceAvailableChanged FINAL)

public:
    explicit InputCapabilities(QObject* parent = nullptr);
    ~InputCapabilities() override = default;

    bool touchDeviceAvailable() const { return _touchDeviceAvailable; }

    Q_INVOKABLE void refresh();

signals:
    void touchDeviceAvailableChanged();

private:
    bool _touchDeviceAvailable = false;
};
