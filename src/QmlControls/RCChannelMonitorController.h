#pragma once

#include "FactPanelController.h"

#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(RCChannelMonitorControllerLog)

class RCChannelMonitorController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int channelCount READ channelCount NOTIFY channelCountChanged)
    Q_PROPERTY(bool clampValues READ clampValues WRITE setClampValues NOTIFY clampValuesChanged)

public:
    explicit RCChannelMonitorController(QObject *parent = nullptr);
    ~RCChannelMonitorController();

    int channelCount() const { return _chanCount; }
    bool clampValues() const { return _clampValues; }
    void setClampValues(bool clamp);

signals:
    void channelCountChanged(int channelCount);
    void channelValueChanged(int channel, int rcValue);
    void clampValuesChanged();

private slots:
    void channelValuesChanged(QVector<int> pwmValues);

private:
    void _connectToSignal();

    int _chanCount = 0;
    bool _clampValues = true;
};
