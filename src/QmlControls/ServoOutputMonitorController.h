#pragma once

#include "FactPanelController.h"

#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(ServoOutputMonitorControllerLog)

class ServoOutputMonitorController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int servoCount READ servoCount NOTIFY servoCountChanged)

public:
    explicit ServoOutputMonitorController(QObject *parent = nullptr);
    ~ServoOutputMonitorController();

    int servoCount() const { return _servoCount; }

    /// Servo index is 0..15 (SERVO1..SERVO16)
    Q_INVOKABLE int servoValue(int servoIndex) const;

signals:
    void servoCountChanged(int servoCount);
    void servoValueChanged(int servo, int pwmValue);

private slots:
    void servoValuesChanged(QVector<int> pwmValues);

private:
    int _servoCount = 0;
    QVector<int> _servoValues = QVector<int>(16, -1);
};
