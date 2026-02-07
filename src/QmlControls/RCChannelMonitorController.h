#pragma once

#include "FactPanelController.h"
#include "QGCMAVLink.h"

#include <QtQmlIntegration/QtQmlIntegration>


class RCChannelMonitorController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int channelCount READ channelCount NOTIFY channelCountChanged)

public:
    explicit RCChannelMonitorController(QObject *parent = nullptr);
    ~RCChannelMonitorController();

    int channelCount() const { return _chanCount; }

signals:
    void channelCountChanged(int channelCount);
    void channelValueChanged(int channel, int rcValue);

private slots:
    void channelValuesChanged(QVector<int> pwmValues);

private:
    int _chanCount = 0;
};
