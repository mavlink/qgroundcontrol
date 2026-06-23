#include "ServoOutputMonitorController.h"

#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(ServoOutputMonitorControllerLog, "QMLControls.ServoOutputMonitorController")

ServoOutputMonitorController::ServoOutputMonitorController(QObject *parent)
    : FactPanelController(parent)
{
    (void) connect(_vehicle, &Vehicle::servoOutputsChanged, this, &ServoOutputMonitorController::servoValuesChanged);
}

ServoOutputMonitorController::~ServoOutputMonitorController()
{
}

int ServoOutputMonitorController::servoValue(int servoIndex) const
{
    if (servoIndex >= 0 && servoIndex < _servoValues.size()) {
        return _servoValues[servoIndex];
    }

    return -1;
}

void ServoOutputMonitorController::servoValuesChanged(QVector<int> pwmValues)
{
    const int servoCount = pwmValues.size();

    _servoValues = pwmValues;

    if (_servoCount != servoCount) {
        _servoCount = servoCount;
        emit servoCountChanged(_servoCount);
    }

    for (int servo = 0; servo < servoCount; servo++) {
        emit servoValueChanged(servo, pwmValues[servo]);
    }
}
