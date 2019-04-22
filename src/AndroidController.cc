/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AndroidInterface.h"
#include "AndroidController.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(AndroidControllerLog, "AndroidControllerLog")

AndroidController::AndroidController(QGCApplication* app)
    : QObject()
    , _multiVehicleManager(NULL)
    , _activeVehicle(NULL)
    , _appActive(true)
{
    if (app != NULL && app->toolbox() != NULL) {
        _multiVehicleManager = app->toolbox()->multiVehicleManager();
        if (_multiVehicleManager != NULL) {
            _activeVehicle = _multiVehicleManager->activeVehicle();
            connect(_multiVehicleManager, &MultiVehicleManager::activeVehicleChanged, this, &AndroidController::_activeVehicleChanged);
            if (_activeVehicle != NULL) {
                connect(_activeVehicle, &Vehicle::armedChanged, this, &AndroidController::_vehicleArmedChanged);
            }
        }
    }
    QtAndroidPrivate::registerKeyEventListener(this);
    ACTION_DOWN = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "ACTION_DOWN");
    ACTION_UP = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "ACTION_UP");
    KEYCODE_CAMERA = QAndroidJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_CAMERA");
    connect(this, &AndroidController::cameraKeyClicked, this, &AndroidController::_cameraKeyClickedHandler, Qt::QueuedConnection);
    connect(app, &QGCApplication::applicationStateChanged, this, &AndroidController::_handleApplicationStateChanged);
}

AndroidController::~AndroidController()
{
    QtAndroidPrivate::unregisterKeyEventListener(this);
}

bool AndroidController::handleKeyEvent(jobject event)
{
    QJNIObjectPrivate ev(event);
    const int keyCode = ev.callMethod<jint>("getKeyCode", "()I");
    if (keyCode != KEYCODE_CAMERA) {
        return false;
    }
    const int action = ev.callMethod<jint>("getAction", "()I");
    if (action == ACTION_DOWN) {
        emit cameraKeyPressed();
    } else if (action == ACTION_UP) {
        emit cameraKeyClicked();
    }
    return false;
}

void AndroidController::_activeVehicleChanged(Vehicle* activeVehicle)
{
    if (_activeVehicle) {
        disconnect(_activeVehicle, &Vehicle::armedChanged, this, &AndroidController::_vehicleArmedChanged);
        _activeVehicle = activeVehicle;
        if(_activeVehicle) {
            connect(_activeVehicle, &Vehicle::armedChanged, this, &AndroidController::_vehicleArmedChanged);
        } else {
            AndroidInterface::releaseScreenWakeLock();
        }
    } else if (activeVehicle){
        qCDebug(AndroidControllerLog) << "Change ot new active vehicle";
        _activeVehicle = activeVehicle;
        connect(_activeVehicle, &Vehicle::armedChanged, this, &AndroidController::_vehicleArmedChanged);
        _vehicleArmedChanged(_activeVehicle->armed());
    }
}

void AndroidController::_vehicleArmedChanged(bool armed)
{
    qCDebug(AndroidControllerLog) << "Vehicle armed state changed to:" << armed;
    if (armed && _appActive) {
        AndroidInterface::acquireScreenWakeLock();
    } else {
        AndroidInterface::releaseScreenWakeLock();
    }
}

void AndroidController::_cameraKeyClickedHandler()
{
//    if(_activeVehicle) {
//        _activeVehicle->triggerCamera();
//    }
}

void AndroidController::_handleApplicationStateChanged(Qt::ApplicationState state)
{
    if(state == Qt::ApplicationActive) {
        _appActive = true;
        if (_activeVehicle && _activeVehicle->armed()) {
            qCDebug(AndroidControllerLog) << "acquire wake lock when resume";
            AndroidInterface::acquireScreenWakeLock();
        }
    } else if (state == Qt::ApplicationInactive) {
        _appActive = false;
        qCDebug(AndroidControllerLog) << "release wake lock when app in background";
        AndroidInterface::releaseScreenWakeLock();
    }
}
