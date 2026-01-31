#pragma once

#include "FactPanelController.h"
#include "OrientationCalibrationState.h"

#include <QtQuick/QQuickItem>
#include <QtQmlIntegration/QtQmlIntegration>

class SensorCalibrationStateMachineBase;

/// Base class for sensor calibration controllers.
/// Provides common functionality for APM and PX4 sensor calibration.
class SensorCalibrationControllerBase : public FactPanelController
{
    Q_OBJECT

    friend class SensorCalibrationStateMachineBase;

public:
    explicit SensorCalibrationControllerBase(QObject* parent = nullptr);
    ~SensorCalibrationControllerBase() override = default;

    // UI element properties (bound from QML)
    Q_PROPERTY(QQuickItem* statusLog MEMBER _statusLog)
    Q_PROPERTY(QQuickItem* progressBar MEMBER _progressBar)
    Q_PROPERTY(QQuickItem* cancelButton MEMBER _cancelButton)
    Q_PROPERTY(QQuickItem* orientationCalAreaHelpText MEMBER _orientationCalAreaHelpText)

    // Orientation calibration area visibility
    Q_PROPERTY(bool showOrientationCalArea READ showOrientationCalArea NOTIFY showOrientationCalAreaChanged)

    // Side state properties for QML binding
    Q_PROPERTY(bool orientationCalDownSideDone READ orientationCalDownSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideDone READ orientationCalUpsideDownSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalLeftSideDone READ orientationCalLeftSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalRightSideDone READ orientationCalRightSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideDone READ orientationCalNoseDownSideDone NOTIFY orientationCalSidesDoneChanged)
    Q_PROPERTY(bool orientationCalTailDownSideDone READ orientationCalTailDownSideDone NOTIFY orientationCalSidesDoneChanged)

    Q_PROPERTY(bool orientationCalDownSideVisible READ orientationCalDownSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideVisible READ orientationCalUpsideDownSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalLeftSideVisible READ orientationCalLeftSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalRightSideVisible READ orientationCalRightSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideVisible READ orientationCalNoseDownSideVisible NOTIFY orientationCalSidesVisibleChanged)
    Q_PROPERTY(bool orientationCalTailDownSideVisible READ orientationCalTailDownSideVisible NOTIFY orientationCalSidesVisibleChanged)

    Q_PROPERTY(bool orientationCalDownSideInProgress READ orientationCalDownSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideInProgress READ orientationCalUpsideDownSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalLeftSideInProgress READ orientationCalLeftSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalRightSideInProgress READ orientationCalRightSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideInProgress READ orientationCalNoseDownSideInProgress NOTIFY orientationCalSidesInProgressChanged)
    Q_PROPERTY(bool orientationCalTailDownSideInProgress READ orientationCalTailDownSideInProgress NOTIFY orientationCalSidesInProgressChanged)

    Q_PROPERTY(bool orientationCalDownSideRotate READ orientationCalDownSideRotate NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalUpsideDownSideRotate READ orientationCalUpsideDownSideRotate NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalLeftSideRotate READ orientationCalLeftSideRotate NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalRightSideRotate READ orientationCalRightSideRotate NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalNoseDownSideRotate READ orientationCalNoseDownSideRotate NOTIFY orientationCalSidesRotateChanged)
    Q_PROPERTY(bool orientationCalTailDownSideRotate READ orientationCalTailDownSideRotate NOTIFY orientationCalSidesRotateChanged)

    Q_PROPERTY(bool waitingForCancel READ waitingForCancel NOTIFY waitingForCancelChanged)

    // Property accessors
    bool showOrientationCalArea() const { return _showOrientationCalArea; }
    bool waitingForCancel() const { return _waitingForCancel; }

    // Side state accessors (delegate to OrientationCalibrationState)
    bool orientationCalDownSideDone() const { return _orientationState.downSideDone(); }
    bool orientationCalUpsideDownSideDone() const { return _orientationState.upSideDone(); }
    bool orientationCalLeftSideDone() const { return _orientationState.leftSideDone(); }
    bool orientationCalRightSideDone() const { return _orientationState.rightSideDone(); }
    bool orientationCalNoseDownSideDone() const { return _orientationState.frontSideDone(); }
    bool orientationCalTailDownSideDone() const { return _orientationState.backSideDone(); }

    bool orientationCalDownSideVisible() const { return _orientationState.downSideVisible(); }
    bool orientationCalUpsideDownSideVisible() const { return _orientationState.upSideVisible(); }
    bool orientationCalLeftSideVisible() const { return _orientationState.leftSideVisible(); }
    bool orientationCalRightSideVisible() const { return _orientationState.rightSideVisible(); }
    bool orientationCalNoseDownSideVisible() const { return _orientationState.frontSideVisible(); }
    bool orientationCalTailDownSideVisible() const { return _orientationState.backSideVisible(); }

    bool orientationCalDownSideInProgress() const { return _orientationState.downSideInProgress(); }
    bool orientationCalUpsideDownSideInProgress() const { return _orientationState.upSideInProgress(); }
    bool orientationCalLeftSideInProgress() const { return _orientationState.leftSideInProgress(); }
    bool orientationCalRightSideInProgress() const { return _orientationState.rightSideInProgress(); }
    bool orientationCalNoseDownSideInProgress() const { return _orientationState.frontSideInProgress(); }
    bool orientationCalTailDownSideInProgress() const { return _orientationState.backSideInProgress(); }

    bool orientationCalDownSideRotate() const { return _orientationState.downSideRotate(); }
    bool orientationCalUpsideDownSideRotate() const { return _orientationState.upSideRotate(); }
    bool orientationCalLeftSideRotate() const { return _orientationState.leftSideRotate(); }
    bool orientationCalRightSideRotate() const { return _orientationState.rightSideRotate(); }
    bool orientationCalNoseDownSideRotate() const { return _orientationState.frontSideRotate(); }
    bool orientationCalTailDownSideRotate() const { return _orientationState.backSideRotate(); }

    /// Get the orientation state tracker
    OrientationCalibrationState& orientationState() { return _orientationState; }
    const OrientationCalibrationState& orientationState() const { return _orientationState; }

    /// Set waiting for cancel state (called by state machines)
    void setWaitingForCancel(bool waiting);

signals:
    void showOrientationCalAreaChanged();
    void orientationCalSidesDoneChanged();
    void orientationCalSidesVisibleChanged();
    void orientationCalSidesInProgressChanged();
    void orientationCalSidesRotateChanged();
    void waitingForCancelChanged();
    void resetStatusTextArea();
    void magCalComplete();

protected:
    /// Append text to the status log
    void appendStatusLog(const QString& text);

    /// Set progress bar value (0.0 to 1.0)
    void setProgress(float value);

    /// Set help text in orientation cal area
    void setOrientationHelpText(const QString& text);

    /// Show or hide the orientation calibration area
    void setShowOrientationCalArea(bool show);

    /// Hide all calibration areas
    void hideAllCalAreas();

    /// Enable or disable buttons during calibration
    virtual void setButtonsEnabled(bool enabled) = 0;

    // UI elements
    QQuickItem* _statusLog = nullptr;
    QQuickItem* _progressBar = nullptr;
    QQuickItem* _cancelButton = nullptr;
    QQuickItem* _orientationCalAreaHelpText = nullptr;

    // State tracking
    OrientationCalibrationState _orientationState;
    bool _showOrientationCalArea = false;
    bool _waitingForCancel = false;
};
