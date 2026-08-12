/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Cinematic Startup Controller
 *
 ****************************************************************************/

#include "VoladorStartupController.h"
#include <QtCore/QDebug>

namespace Volador {

VoladorStartupController::VoladorStartupController(QObject *parent)
    : QObject(parent)
    , _initTimer(new QTimer(this))
{
    _initTimer->setSingleShot(false);
    _initTimer->setInterval(50);
    connect(_initTimer, &QTimer::timeout, this, &VoladorStartupController::simulateInitialization);
}

void VoladorStartupController::setIsReady(bool ready) {
    if (_isReady != ready) {
        _isReady = ready;
        emit isReadyChanged(_isReady);
    }
}

void VoladorStartupController::setIsCompleted(bool completed) {
    if (_isCompleted != completed) {
        _isCompleted = completed;
        emit isCompletedChanged(_isCompleted);
    }
}

void VoladorStartupController::setProgress(double progress) {
    if (qAbs(_progress - progress) > 0.001) {
        _progress = qBound(0.0, progress, 1.0);
        emit progressChanged(_progress);
    }
}

void VoladorStartupController::startSequence() {
    qInfo() << "[VoladorStartupController] Starting cinematic application startup sequence...";
    setIsCompleted(false);
    setIsReady(false);
    setProgress(0.0);
    emit sequenceStarted();
    _initTimer->start();
}

void VoladorStartupController::simulateInitialization() {
    double current = progress() + 0.05;
    setProgress(current);
    if (current >= 1.0) {
        _initTimer->stop();
        setIsReady(true);
        qInfo() << "[VoladorStartupController] Engine background initialization complete.";
    }
}

void VoladorStartupController::completeStartup() {
    qInfo() << "[VoladorStartupController] Cinematic startup transition complete into application shell.";
    setIsCompleted(true);
    emit sequenceFinished();
}

} // namespace Volador
