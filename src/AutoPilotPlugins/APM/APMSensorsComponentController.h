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

/// Sensors Component MVC Controller for SensorsComponent.qml.
class APMSensorsComponentController : public FactPanelController
{
    Q_OBJECT
    
public:
    APMSensorsComponentController(void);

    Q_PROPERTY(QQuickItem* statusLog MEMBER _statusLog)
    Q_PROPERTY(QQuickItem* progressBar MEMBER _progressBar)
    
    Q_PROPERTY(QQuickItem* compassButton MEMBER _compassButton)
    Q_PROPERTY(QQuickItem* accelButton MEMBER _accelButton)
    Q_PROPERTY(QQuickItem* nextButton MEMBER _nextButton)
    Q_PROPERTY(QQuickItem* cancelButton MEMBER _cancelButton)
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
    
    Q_INVOKABLE void calibrateCompass(void);
    Q_INVOKABLE void calibrateAccel(void);
    Q_INVOKABLE void cancelCalibration(void);
    Q_INVOKABLE void nextClicked(void);

    bool compassSetupNeeded(void) const;
    bool accelSetupNeeded(void) const;
    
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
    void calibrationComplete(void);
    
private slots:
    void _handleUASTextMessage(int uasId, int compId, int severity, QString text);
    
private:
    void _startLogCalibration(void);
    void _startVisualCalibration(void);
    void _appendStatusLog(const QString& text);
    void _refreshParams(void);
    void _hideAllCalAreas(void);
    void _resetInternalState(void);
    
    enum StopCalibrationCode {
        StopCalibrationSuccess,
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
    QQuickItem* _nextButton;
    QQuickItem* _cancelButton;
    QQuickItem* _orientationCalAreaHelpText;
    
    bool _showOrientationCalArea;
    
    bool _magCalInProgress;
    bool _accelCalInProgress;
    
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
    
    static const int _supportedFirmwareCalVersion = 2;
};

#endif
