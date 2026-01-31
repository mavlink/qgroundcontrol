#pragma once

#include <QtQuick/QQuickItem>
#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

#include "SensorCalibrationControllerBase.h"

Q_DECLARE_LOGGING_CATEGORY(SensorsComponentControllerLog)

class PX4SensorCalibrationStateMachine;
class PX4OrientationCalibrationMachine;

/// Sensors Component MVC Controller for SensorsComponent.qml.
class SensorsComponentController : public SensorCalibrationControllerBase
{
    Q_OBJECT
    QML_ELEMENT

    friend class PX4SensorCalibrationStateMachine;
    friend class PX4OrientationCalibrationMachine;

public:
    SensorsComponentController(void);

    Q_PROPERTY(QQuickItem* compassButton MEMBER _compassButton)
    Q_PROPERTY(QQuickItem* gyroButton MEMBER _gyroButton)
    Q_PROPERTY(QQuickItem* accelButton MEMBER _accelButton)
    Q_PROPERTY(QQuickItem* airspeedButton MEMBER _airspeedButton)
    Q_PROPERTY(QQuickItem* levelButton MEMBER _levelButton)
    Q_PROPERTY(QQuickItem* setOrientationsButton MEMBER _setOrientationsButton)

    Q_INVOKABLE void calibrateCompass(void);
    Q_INVOKABLE void calibrateGyro(void);
    Q_INVOKABLE void calibrateAccel(void);
    Q_INVOKABLE void calibrateLevel(void);
    Q_INVOKABLE void calibrateAirspeed(void);
    Q_INVOKABLE void cancelCalibration(void);
    Q_INVOKABLE bool usingUDPLink(void);
    Q_INVOKABLE void resetFactoryParameters();

signals:
    void showGyroCalAreaChanged(void);

protected:
    void setButtonsEnabled(bool enabled) override;

private slots:
    void _handleUASTextMessage(int uasId, int compId, int severity, QString text, const QString &description);
    void _handleParametersReset(bool success);

private:
    void _refreshParams(void);
    void _resetInternalState(void);

    QQuickItem* _compassButton = nullptr;
    QQuickItem* _gyroButton = nullptr;
    QQuickItem* _accelButton = nullptr;
    QQuickItem* _airspeedButton = nullptr;
    QQuickItem* _levelButton = nullptr;
    QQuickItem* _setOrientationsButton = nullptr;

    bool _showGyroCalArea = false;

    bool _gyroCalInProgress = false;
    bool _magCalInProgress = false;
    bool _accelCalInProgress = false;
    bool _airspeedCalInProgress = false;
    bool _levelCalInProgress = false;

    bool _unknownFirmwareVersion = false;

    PX4SensorCalibrationStateMachine* _stateMachine = nullptr;
};
