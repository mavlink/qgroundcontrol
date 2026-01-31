#include "SensorCalibrationControllerBase.h"

#include <QtCore/QMetaObject>

SensorCalibrationControllerBase::SensorCalibrationControllerBase(QObject* parent)
    : FactPanelController(parent)
    , _orientationState(this)
{
    // Connect orientation state signals to controller signals
    connect(&_orientationState, &OrientationCalibrationState::sidesDoneChanged,
            this, &SensorCalibrationControllerBase::orientationCalSidesDoneChanged);
    connect(&_orientationState, &OrientationCalibrationState::sidesVisibleChanged,
            this, &SensorCalibrationControllerBase::orientationCalSidesVisibleChanged);
    connect(&_orientationState, &OrientationCalibrationState::sidesInProgressChanged,
            this, &SensorCalibrationControllerBase::orientationCalSidesInProgressChanged);
    connect(&_orientationState, &OrientationCalibrationState::sidesRotateChanged,
            this, &SensorCalibrationControllerBase::orientationCalSidesRotateChanged);
}

void SensorCalibrationControllerBase::appendStatusLog(const QString& text)
{
    if (!_statusLog) {
        qWarning() << "Status log not set";
        return;
    }

    QString varText = text;
    QMetaObject::invokeMethod(_statusLog, "append", Q_ARG(QString, varText));
}

void SensorCalibrationControllerBase::setProgress(float value)
{
    if (_progressBar) {
        (void) _progressBar->setProperty("value", value);
    }
}

void SensorCalibrationControllerBase::setOrientationHelpText(const QString& text)
{
    if (_orientationCalAreaHelpText) {
        (void) _orientationCalAreaHelpText->setProperty("text", text);
    }
}

void SensorCalibrationControllerBase::setShowOrientationCalArea(bool show)
{
    if (_showOrientationCalArea != show) {
        _showOrientationCalArea = show;
        emit showOrientationCalAreaChanged();
    }
}

void SensorCalibrationControllerBase::hideAllCalAreas()
{
    setShowOrientationCalArea(false);
}

void SensorCalibrationControllerBase::setWaitingForCancel(bool waiting)
{
    if (_waitingForCancel != waiting) {
        _waitingForCancel = waiting;
        emit waitingForCancelChanged();
    }
}
