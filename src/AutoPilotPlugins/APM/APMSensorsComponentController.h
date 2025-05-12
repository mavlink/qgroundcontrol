/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactPanelController.h"
#include "QGCMAVLink.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtQuick/QQuickItem>

Q_DECLARE_LOGGING_CATEGORY(APMSensorsComponentControllerLog)
Q_DECLARE_LOGGING_CATEGORY(APMSensorsComponentControllerVerboseLog)

class APMSensorsComponent;
class LinkInterface;

/// Sensors Component MVC Controller for SensorsComponent.qml.
class APMSensorsComponentController : public FactPanelController
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem* statusLog                        MEMBER _statusLog)
    Q_PROPERTY(QQuickItem* progressBar                      MEMBER _progressBar)

    Q_PROPERTY(QQuickItem* nextButton                       MEMBER _nextButton)
    Q_PROPERTY(QQuickItem* cancelButton                     MEMBER _cancelButton)
    Q_PROPERTY(QQuickItem* orientationCalAreaHelpText       MEMBER _orientationCalAreaHelpText)

    Q_PROPERTY(bool compassSetupNeeded                      READ compassSetupNeeded                         NOTIFY setupNeededChanged)
    Q_PROPERTY(bool accelSetupNeeded                        READ accelSetupNeeded                           NOTIFY setupNeededChanged)

    Q_PROPERTY(bool showOrientationCalArea                  MEMBER _showOrientationCalArea                  NOTIFY showOrientationCalAreaChanged)

    Q_PROPERTY(bool orientationCalDownSideDone              MEMBER _orientationCalDownSideDone              NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideDone        MEMBER _orientationCalUpsideDownSideDone        NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalLeftSideDone              MEMBER _orientationCalLeftSideDone              NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalRightSideDone             MEMBER _orientationCalRightSideDone             NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideDone          MEMBER _orientationCalNoseDownSideDone          NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalTailDownSideDone          MEMBER _orientationCalTailDownSideDone          NOTIFY orientationCalSidesDoneChanged)

    Q_PROPERTY(bool orientationCalDownSideVisible           MEMBER _orientationCalDownSideVisible           NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideVisible     MEMBER _orientationCalUpsideDownSideVisible     NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalLeftSideVisible           MEMBER _orientationCalLeftSideVisible           NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalRightSideVisible          MEMBER _orientationCalRightSideVisible          NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideVisible       MEMBER _orientationCalNoseDownSideVisible       NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalTailDownSideVisible       MEMBER _orientationCalTailDownSideVisible       NOTIFY orientationCalSidesVisibleChanged)

    Q_PROPERTY(bool orientationCalDownSideInProgress        MEMBER _orientationCalDownSideInProgress        NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideInProgress  MEMBER _orientationCalUpsideDownSideInProgress  NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalLeftSideInProgress        MEMBER _orientationCalLeftSideInProgress        NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalRightSideInProgress       MEMBER _orientationCalRightSideInProgress       NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideInProgress    MEMBER _orientationCalNoseDownSideInProgress    NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalTailDownSideInProgress    MEMBER _orientationCalTailDownSideInProgress    NOTIFY orientationCalSidesInProgressChanged)

    Q_PROPERTY(bool orientationCalDownSideRotate            MEMBER _orientationCalDownSideRotate            NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideRotate      MEMBER _orientationCalUpsideDownSideRotate      NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalLeftSideRotate            MEMBER _orientationCalLeftSideRotate            NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalRightSideRotate           MEMBER _orientationCalRightSideRotate           NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideRotate        MEMBER _orientationCalNoseDownSideRotate        NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalTailDownSideRotate        MEMBER _orientationCalTailDownSideRotate        NOTIFY orientationCalSidesRotateChanged)

    Q_PROPERTY(bool waitingForCancel                        MEMBER _waitingForCancel                        NOTIFY waitingForCancelChanged)

    Q_PROPERTY(bool compass1CalSucceeded                    READ compass1CalSucceeded                       NOTIFY compass1CalSucceededChanged)
    Q_PROPERTY(bool compass2CalSucceeded                    READ compass2CalSucceeded                       NOTIFY compass2CalSucceededChanged)
    Q_PROPERTY(bool compass3CalSucceeded                    READ compass3CalSucceeded                       NOTIFY compass3CalSucceededChanged)

    Q_PROPERTY(double compass1CalFitness                    READ compass1CalFitness                         NOTIFY compass1CalFitnessChanged)
    Q_PROPERTY(double compass2CalFitness                    READ compass2CalFitness                         NOTIFY compass2CalFitnessChanged)
    Q_PROPERTY(double compass3CalFitness                    READ compass3CalFitness                         NOTIFY compass3CalFitnessChanged)

public:
    explicit APMSensorsComponentController(QObject *parent = nullptr);
    ~APMSensorsComponentController();

    Q_INVOKABLE void calibrateCompass();
    Q_INVOKABLE void calibrateAccel(bool doSimpleAccelCal);
    Q_INVOKABLE void calibrateCompassNorth(float lat, float lon, int mask);
    Q_INVOKABLE void calibrateGyro();
    Q_INVOKABLE void calibrateMotorInterference();
    Q_INVOKABLE void levelHorizon();
    Q_INVOKABLE void calibratePressure();
    Q_INVOKABLE void cancelCalibration();
    Q_INVOKABLE void nextClicked();
    Q_INVOKABLE bool usingUDPLink() const;

    bool compassSetupNeeded() const;
    bool accelSetupNeeded() const;

    bool compass1CalSucceeded() const { return _rgCompassCalSucceeded[0]; }
    bool compass2CalSucceeded() const { return _rgCompassCalSucceeded[1]; }
    bool compass3CalSucceeded() const { return _rgCompassCalSucceeded[2]; }

    double compass1CalFitness() const { return _rgCompassCalFitness[0]; }
    double compass2CalFitness() const { return _rgCompassCalFitness[1]; }
    double compass3CalFitness() const { return _rgCompassCalFitness[2]; }

signals:
    void showGyroCalAreaChanged();
    void showOrientationCalAreaChanged();
    void orientationCalSidesDoneChanged();
    void orientationCalSidesVisibleChanged();
    void orientationCalSidesInProgressChanged();
    void orientationCalSidesRotateChanged();
    void resetStatusTextArea();
    void waitingForCancelChanged();
    void setupNeededChanged();
    void calibrationComplete(QGCMAVLink::CalibrationType calType);
    void compass1CalSucceededChanged(bool compass1CalSucceeded);
    void compass2CalSucceededChanged(bool compass2CalSucceeded);
    void compass3CalSucceededChanged(bool compass3CalSucceeded);
    void compass1CalFitnessChanged(double compass1CalFitness);
    void compass2CalFitnessChanged(double compass2CalFitness);
    void compass3CalFitnessChanged(double compass3CalFitness);
    void setAllCalButtonsEnabled(bool enabled);

private slots:
    void _handleTextMessage(int sysid, int componentid, int severity, const QString &text, const QString &description);
    void _mavlinkMessageReceived(LinkInterface *link, const mavlink_message_t &message);
    void _mavCommandResult(int vehicleId, int component, int command, int result, int failureCode);

private:
    void _startLogCalibration();
    void _startVisualCalibration();
    /// Appends the specified text to the status log area in the ui
    void _appendStatusLog(const QString &text);
    void _refreshParams();
    void _hideAllCalAreas();
    void _resetInternalState();
    void _handleCommandAck(const mavlink_message_t &message);
    void _handleMagCalProgress(const mavlink_message_t &message);
    void _handleMagCalReport(const mavlink_message_t &message);
    bool _handleCmdLongAccelcalVehiclePos(const mavlink_command_long_t &commandLong);
    void _handleCommandLong(const mavlink_message_t &message);
    void _restorePreviousCompassCalFitness();

    enum StopCalibrationCode {
        StopCalibrationSuccess,
        StopCalibrationSuccessShowLog,
        StopCalibrationFailed,
        StopCalibrationCancelled
    };
    void _stopCalibration(StopCalibrationCode code);

    void _updateAndEmitShowOrientationCalArea(bool show);

    APMSensorsComponent *_sensorsComponent = nullptr;

    QQuickItem *_statusLog = nullptr;
    QQuickItem *_progressBar = nullptr;
    QQuickItem *_nextButton = nullptr;
    QQuickItem *_cancelButton = nullptr;
    QQuickItem *_orientationCalAreaHelpText = nullptr;

    bool _showOrientationCalArea = false;

    QGCMAVLink::CalibrationType _calTypeInProgress = QGCMAVLink::CalibrationNone;

    uint8_t _rgCompassCalProgress[3];
    bool _rgCompassCalComplete[3];
    bool _rgCompassCalSucceeded[3];
    float _rgCompassCalFitness[3];

    bool _orientationCalDownSideDone = false;;
    bool _orientationCalUpsideDownSideDone = false;;
    bool _orientationCalLeftSideDone = false;;
    bool _orientationCalRightSideDone = false;;
    bool _orientationCalNoseDownSideDone = false;;
    bool _orientationCalTailDownSideDone = false;;

    bool _orientationCalDownSideVisible = false;;
    bool _orientationCalUpsideDownSideVisible = false;;
    bool _orientationCalLeftSideVisible = false;;
    bool _orientationCalRightSideVisible = false;;
    bool _orientationCalNoseDownSideVisible = false;;
    bool _orientationCalTailDownSideVisible = false;;

    bool _orientationCalDownSideInProgress = false;;
    bool _orientationCalUpsideDownSideInProgress = false;;
    bool _orientationCalLeftSideInProgress = false;;
    bool _orientationCalRightSideInProgress = false;;
    bool _orientationCalNoseDownSideInProgress = false;;
    bool _orientationCalTailDownSideInProgress = false;;

    bool _orientationCalDownSideRotate = false;;
    bool _orientationCalUpsideDownSideRotate = false;;
    bool _orientationCalLeftSideRotate = false;;
    bool _orientationCalRightSideRotate = false;;
    bool _orientationCalNoseDownSideRotate = false;;
    bool _orientationCalTailDownSideRotate = false;;

    bool _waitingForCancel = false;

    bool _restoreCompassCalFitness = false;
    float _previousCompassCalFitness = 0.;
    static constexpr const char *_compassCalFitnessParam = "COMPASS_CAL_FIT";

    static constexpr int _supportedFirmwareCalVersion = 2;
};
