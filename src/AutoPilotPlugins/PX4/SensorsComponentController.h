/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef SENSORSCOMPONENTCONTROLLER_H
#define SENSORSCOMPONENTCONTROLLER_H

#include <QObject>
#include <QQuickItem>

#include "UASInterface.h"
#include "FactPanelController.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(SensorsComponentControllerLog)

/// Sensors Component MVC Controller for SensorsComponent.qml.
class SensorsComponentController : public FactPanelController
{
    Q_OBJECT
    
public:
    SensorsComponentController(void);
    
    Q_PROPERTY(QQuickItem* statusLog MEMBER _statusLog)
    Q_PROPERTY(QQuickItem* progressBar MEMBER _progressBar)
    
    Q_PROPERTY(QQuickItem* compassButton MEMBER _compassButton)
    Q_PROPERTY(QQuickItem* gyroButton MEMBER _gyroButton)
    Q_PROPERTY(QQuickItem* accelButton MEMBER _accelButton)
    Q_PROPERTY(QQuickItem* airspeedButton MEMBER _airspeedButton)
    Q_PROPERTY(QQuickItem* levelButton MEMBER _levelButton)
    Q_PROPERTY(QQuickItem* cancelButton MEMBER _cancelButton)
    Q_PROPERTY(QQuickItem* setOrientationsButton MEMBER _setOrientationsButton)
    Q_PROPERTY(QQuickItem* orientationCalAreaHelpText MEMBER _orientationCalAreaHelpText)
    
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
    Q_INVOKABLE void calibrateGyro(void);
    Q_INVOKABLE void calibrateAccel(void);
    Q_INVOKABLE void calibrateLevel(void);
    Q_INVOKABLE void calibrateAirspeed(void);
    Q_INVOKABLE void cancelCalibration(void);
    Q_INVOKABLE bool usingUDPLink(void);
    
signals:
    void showGyroCalAreaChanged(void);
    void showOrientationCalAreaChanged(void);
    void orientationCalSidesDoneChanged(void);
    void orientationCalSidesVisibleChanged(void);
    void orientationCalSidesInProgressChanged(void);
    void orientationCalSidesRotateChanged(void);
    void resetStatusTextArea(void);
    void waitingForCancelChanged(void);
    void magCalComplete(void);

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

    QQuickItem* _statusLog;
    QQuickItem* _progressBar;
    QQuickItem* _compassButton;
    QQuickItem* _gyroButton;
    QQuickItem* _accelButton;
    QQuickItem* _airspeedButton;
    QQuickItem* _levelButton;
    QQuickItem* _cancelButton;
    QQuickItem* _setOrientationsButton;
    QQuickItem* _orientationCalAreaHelpText;
    
    bool _showGyroCalArea;
    bool _showOrientationCalArea;
    
    bool _gyroCalInProgress;
    bool _magCalInProgress;
    bool _accelCalInProgress;
    bool _airspeedCalInProgress;
    bool _levelCalInProgress;

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
    
    bool _unknownFirmwareVersion;
    bool _waitingForCancel;
    
    static const int _supportedFirmwareCalVersion = 2;
};

#endif
