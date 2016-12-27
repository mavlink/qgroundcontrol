/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef APMSensorsComponentController_H
#define APMSensorsComponentController_H

#include <QObject>

#include "UASInterface.h"
#include "FactPanelController.h"
#include "QGCLoggingCategory.h"
#include "APMSensorsComponent.h"
#include "APMCompassCal.h"

Q_DECLARE_LOGGING_CATEGORY(APMSensorsComponentControllerLog)
Q_DECLARE_LOGGING_CATEGORY(APMSensorsComponentControllerVerboseLog)

/// Sensors Component MVC Controller for SensorsComponent.qml.
class APMSensorsComponentController : public FactPanelController
{
    Q_OBJECT
    
public:
    APMSensorsComponentController(void);
    ~APMSensorsComponentController();

    Q_PROPERTY(QQuickItem* statusLog MEMBER _statusLog)
    Q_PROPERTY(QQuickItem* progressBar MEMBER _progressBar)
    
    Q_PROPERTY(QQuickItem* compassButton MEMBER _compassButton)
    Q_PROPERTY(QQuickItem* accelButton MEMBER _accelButton)
    Q_PROPERTY(QQuickItem* compassMotButton MEMBER _compassMotButton)
    Q_PROPERTY(QQuickItem* levelButton MEMBER _levelButton)
    Q_PROPERTY(QQuickItem* nextButton MEMBER _nextButton)
    Q_PROPERTY(QQuickItem* cancelButton MEMBER _cancelButton)
    Q_PROPERTY(QQuickItem* setOrientationsButton MEMBER _setOrientationsButton)
    Q_PROPERTY(QQuickItem* orientationCalAreaHelpText MEMBER _orientationCalAreaHelpText)

    Q_PROPERTY(bool compassSetupNeeded  READ compassSetupNeeded NOTIFY setupNeededChanged)
    Q_PROPERTY(bool accelSetupNeeded    READ accelSetupNeeded   NOTIFY setupNeededChanged)

    Q_PROPERTY(bool showOrientationCalArea MEMBER _showOrientationCalArea NOTIFY showOrientationCalAreaChanged)
    
    Q_PROPERTY(bool orientationCalDownSideDone MEMBER _orientationCalDownSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideDone MEMBER _orientationCalUpsideDownSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalLeftSideDone MEMBER _orientationCalLeftSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalRightSideDone MEMBER _orientationCalRightSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideDone MEMBER _orientationCalNoseDownSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalTailDownSideDone MEMBER _orientationCalTailDownSideDone NOTIFY orientationCalSidesDoneChanged)
    
    Q_PROPERTY(bool orientationCalDownSideVisible MEMBER _orientationCalDownSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideVisible MEMBER _orientationCalUpsideDownSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalLeftSideVisible MEMBER _orientationCalLeftSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalRightSideVisible MEMBER _orientationCalRightSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideVisible MEMBER _orientationCalNoseDownSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalTailDownSideVisible MEMBER _orientationCalTailDownSideVisible NOTIFY orientationCalSidesVisibleChanged)
    
    Q_PROPERTY(bool orientationCalDownSideInProgress MEMBER _orientationCalDownSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideInProgress MEMBER _orientationCalUpsideDownSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalLeftSideInProgress MEMBER _orientationCalLeftSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalRightSideInProgress MEMBER _orientationCalRightSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideInProgress MEMBER _orientationCalNoseDownSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalTailDownSideInProgress MEMBER _orientationCalTailDownSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    
    Q_PROPERTY(bool orientationCalDownSideRotate MEMBER _orientationCalDownSideRotate NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideRotate MEMBER _orientationCalUpsideDownSideRotate NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalLeftSideRotate MEMBER _orientationCalLeftSideRotate NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalRightSideRotate MEMBER _orientationCalRightSideRotate NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideRotate MEMBER _orientationCalNoseDownSideRotate NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalTailDownSideRotate MEMBER _orientationCalTailDownSideRotate NOTIFY orientationCalSidesRotateChanged)
    
    Q_PROPERTY(bool waitingForCancel MEMBER _waitingForCancel NOTIFY waitingForCancelChanged)

    Q_PROPERTY(bool compass1CalSucceeded READ compass1CalSucceeded NOTIFY compass1CalSucceededChanged)
    Q_PROPERTY(bool compass2CalSucceeded READ compass2CalSucceeded NOTIFY compass2CalSucceededChanged)
    Q_PROPERTY(bool compass3CalSucceeded READ compass3CalSucceeded NOTIFY compass3CalSucceededChanged)

    Q_PROPERTY(double compass1CalFitness READ compass1CalFitness NOTIFY compass1CalFitnessChanged)
    Q_PROPERTY(double compass2CalFitness READ compass2CalFitness NOTIFY compass2CalFitnessChanged)
    Q_PROPERTY(double compass3CalFitness READ compass3CalFitness NOTIFY compass3CalFitnessChanged)

    Q_INVOKABLE void calibrateCompass(void);
    Q_INVOKABLE void calibrateAccel(void);
    Q_INVOKABLE void calibrateMotorInterference(void);
    Q_INVOKABLE void levelHorizon(void);
    Q_INVOKABLE void cancelCalibration(void);
    Q_INVOKABLE void nextClicked(void);
    Q_INVOKABLE bool usingUDPLink(void);

    bool compassSetupNeeded(void) const;
    bool accelSetupNeeded(void) const;

    typedef enum {
        CalTypeAccel,
        CalTypeOnboardCompass,
        CalTypeOffboardCompass,
        CalTypeLevelHorizon,
        CalTypeCompassMot,
        CalTypeNone
    } CalType_t;
    Q_ENUM(CalType_t)

    bool compass1CalSucceeded(void) const { return _rgCompassCalSucceeded[0]; }
    bool compass2CalSucceeded(void) const { return _rgCompassCalSucceeded[1]; }
    bool compass3CalSucceeded(void) const { return _rgCompassCalSucceeded[2]; }

    double compass1CalFitness(void) const { return _rgCompassCalFitness[0]; }
    double compass2CalFitness(void) const { return _rgCompassCalFitness[1]; }
    double compass3CalFitness(void) const { return _rgCompassCalFitness[2]; }

signals:
    void showGyroCalAreaChanged(void);
    void showOrientationCalAreaChanged(void);
    void orientationCalSidesDoneChanged(void);
    void orientationCalSidesVisibleChanged(void);
    void orientationCalSidesInProgressChanged(void);
    void orientationCalSidesRotateChanged(void);
    void resetStatusTextArea(void);
    void waitingForCancelChanged(void);
    void setupNeededChanged(void);
    void calibrationComplete(CalType_t calType);
    void compass1CalSucceededChanged(bool compass1CalSucceeded);
    void compass2CalSucceededChanged(bool compass2CalSucceeded);
    void compass3CalSucceededChanged(bool compass3CalSucceeded);
    void compass1CalFitnessChanged(double compass1CalFitness);
    void compass2CalFitnessChanged(double compass2CalFitness);
    void compass3CalFitnessChanged(double compass3CalFitness);

private slots:
    void _handleUASTextMessage(int uasId, int compId, int severity, QString text);
    void _mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message);
    void _mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle);

private:
    void _startLogCalibration(void);
    void _startVisualCalibration(void);
    void _appendStatusLog(const QString& text);
    void _refreshParams(void);
    void _hideAllCalAreas(void);
    void _resetInternalState(void);
    void _handleCommandAck(mavlink_message_t& message);
    void _handleMagCalProgress(mavlink_message_t& message);
    void _handleMagCalReport(mavlink_message_t& message);
    void _restorePreviousCompassCalFitness(void);

    enum StopCalibrationCode {
        StopCalibrationSuccess,
        StopCalibrationSuccessShowLog,
        StopCalibrationFailed,
        StopCalibrationCancelled
    };
    void _stopCalibration(StopCalibrationCode code);
    
    void _updateAndEmitShowOrientationCalArea(bool show);

    APMCompassCal           _compassCal;
    APMSensorsComponent*    _sensorsComponent;

    QQuickItem* _statusLog;
    QQuickItem* _progressBar;
    QQuickItem* _compassButton;
    QQuickItem* _accelButton;
    QQuickItem* _compassMotButton;
    QQuickItem* _levelButton;
    QQuickItem* _nextButton;
    QQuickItem* _cancelButton;
    QQuickItem* _setOrientationsButton;
    QQuickItem* _orientationCalAreaHelpText;
    
    bool _showOrientationCalArea;
    
    CalType_t _calTypeInProgress;

    uint8_t _rgCompassCalProgress[3];
    bool    _rgCompassCalComplete[3];
    bool    _rgCompassCalSucceeded[3];
    float   _rgCompassCalFitness[3];

    bool _orientationCalDownSideDone;
    bool _orientationCalUpsideDownSideDone;
    bool _orientationCalLeftSideDone;
    bool _orientationCalRightSideDone;
    bool _orientationCalNoseDownSideDone;
    bool _orientationCalTailDownSideDone;
    
    bool _orientationCalDownSideVisible;
    bool _orientationCalUpsideDownSideVisible;
    bool _orientationCalLeftSideVisible;
    bool _orientationCalRightSideVisible;
    bool _orientationCalNoseDownSideVisible;
    bool _orientationCalTailDownSideVisible;
    
    bool _orientationCalDownSideInProgress;
    bool _orientationCalUpsideDownSideInProgress;
    bool _orientationCalLeftSideInProgress;
    bool _orientationCalRightSideInProgress;
    bool _orientationCalNoseDownSideInProgress;
    bool _orientationCalTailDownSideInProgress;
    
    bool _orientationCalDownSideRotate;
    bool _orientationCalUpsideDownSideRotate;
    bool _orientationCalLeftSideRotate;
    bool _orientationCalRightSideRotate;
    bool _orientationCalNoseDownSideRotate;
    bool _orientationCalTailDownSideRotate;
    
    bool _waitingForCancel;

    bool _restoreCompassCalFitness;
    float _previousCompassCalFitness;
    static const char* _compassCalFitnessParam;
    
    static const int _supportedFirmwareCalVersion = 2;
};

#endif
