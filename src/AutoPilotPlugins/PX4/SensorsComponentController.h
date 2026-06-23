#pragma once

#include <QtQuick/QQuickItem>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"

/// \brief Sensors Component MVC Controller for SensorsComponent.qml.
///
class SensorsComponentController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
public:
    SensorsComponentController(void);

    /// Calibration state of a single orientation side shown in the preview.
    /// Values must stay in sync with the VehicleRotationCal.CalState QML enum.
    enum SideCalState {
        SideCalStateIdle,       ///< No calibration data to show (neutral preview)
        SideCalStateIncomplete, ///< Calibration running, side not yet calibrated
        SideCalStateInProgress, ///< Side actively being calibrated
        SideCalStateCompleted   ///< Side calibration complete
    };
    Q_ENUM(SideCalState)

    Q_PROPERTY(QQuickItem* statusLog MEMBER _statusLog)
    Q_PROPERTY(QQuickItem* progressBar MEMBER _progressBar)

    Q_PROPERTY(QQuickItem* orientationCalAreaHelpText MEMBER _orientationCalAreaHelpText)

    Q_PROPERTY(bool calibrationActive READ calibrationActive NOTIFY calibrationActiveChanged)
    Q_PROPERTY(bool magCalInProgress MEMBER _magCalInProgress NOTIFY calibrationActiveChanged)

    Q_PROPERTY(bool showOrientationCalArea MEMBER _showOrientationCalArea NOTIFY showOrientationCalAreaChanged)

    Q_PROPERTY(SideCalState orientationCalDownSideState MEMBER _orientationCalDownSideState NOTIFY orientationCalSidesStateChanged)
    Q_PROPERTY(SideCalState orientationCalUpsideDownSideState MEMBER _orientationCalUpsideDownSideState NOTIFY orientationCalSidesStateChanged)
    Q_PROPERTY(SideCalState orientationCalLeftSideState MEMBER _orientationCalLeftSideState NOTIFY orientationCalSidesStateChanged)
    Q_PROPERTY(SideCalState orientationCalRightSideState MEMBER _orientationCalRightSideState NOTIFY orientationCalSidesStateChanged)
    Q_PROPERTY(SideCalState orientationCalNoseDownSideState MEMBER _orientationCalNoseDownSideState NOTIFY orientationCalSidesStateChanged)
    Q_PROPERTY(SideCalState orientationCalTailDownSideState MEMBER _orientationCalTailDownSideState NOTIFY orientationCalSidesStateChanged)

    Q_PROPERTY(bool orientationCalDownSideVisible MEMBER _orientationCalDownSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideVisible MEMBER _orientationCalUpsideDownSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalLeftSideVisible MEMBER _orientationCalLeftSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalRightSideVisible MEMBER _orientationCalRightSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideVisible MEMBER _orientationCalNoseDownSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalTailDownSideVisible MEMBER _orientationCalTailDownSideVisible NOTIFY orientationCalSidesVisibleChanged)

    Q_PROPERTY(bool waitingForCancel MEMBER _waitingForCancel NOTIFY waitingForCancelChanged)

    Q_INVOKABLE void calibrateCompass(void);
    Q_INVOKABLE void calibrateGyro(void);
    Q_INVOKABLE void calibrateAccel(void);
    Q_INVOKABLE void calibrateLevel(void);
    Q_INVOKABLE void calibrateAirspeed(void);
    Q_INVOKABLE void cancelCalibration(void);
    Q_INVOKABLE bool usingUDPLink(void);
    Q_INVOKABLE void resetFactoryParameters();
    Q_INVOKABLE void resetSidesToIdle(void);

    bool calibrationActive() const { return _magCalInProgress || _gyroCalInProgress || _accelCalInProgress || _airspeedCalInProgress || _levelCalInProgress; }

signals:
    void showGyroCalAreaChanged(void);
    void showOrientationCalAreaChanged(void);
    void orientationCalSidesStateChanged(void);
    void orientationCalSidesVisibleChanged(void);
    void resetStatusTextArea(void);
    void waitingForCancelChanged(void);
    void magCalComplete(void);
    void calibrationActiveChanged(void);

private slots:
    void _handleUASTextMessage(int uasId, int compId, int severity, QString text, const QString &description);
    void _handleParametersReset(bool success);

private:
    void _startLogCalibration(void);
    void _startVisualCalibration(void);
    void _appendStatusLog(const QString& text);
    void _refreshParams(void);
    void _hideAllCalAreas(void);
    void _setAllSidesState(SideCalState state);

    enum StopCalibrationCode {
        StopCalibrationSuccess,
        StopCalibrationFailed,
        StopCalibrationCancelled
    };
    void _stopCalibration(StopCalibrationCode code);

    void _updateAndEmitShowOrientationCalArea(bool show);

    QQuickItem* _statusLog;
    QQuickItem* _progressBar;
    QQuickItem* _orientationCalAreaHelpText;

    bool _showGyroCalArea;
    bool _showOrientationCalArea;

    bool _gyroCalInProgress;
    bool _magCalInProgress;
    bool _accelCalInProgress;
    bool _airspeedCalInProgress;
    bool _levelCalInProgress;

    bool _orientationCalDownSideVisible;
    bool _orientationCalUpsideDownSideVisible;
    bool _orientationCalLeftSideVisible;
    bool _orientationCalRightSideVisible;
    bool _orientationCalNoseDownSideVisible;
    bool _orientationCalTailDownSideVisible;

    SideCalState _orientationCalDownSideState;
    SideCalState _orientationCalUpsideDownSideState;
    SideCalState _orientationCalLeftSideState;
    SideCalState _orientationCalRightSideState;
    SideCalState _orientationCalNoseDownSideState;
    SideCalState _orientationCalTailDownSideState;

    bool _unknownFirmwareVersion;
    bool _waitingForCancel;

    static const int _supportedFirmwareCalVersion = 2;
};
