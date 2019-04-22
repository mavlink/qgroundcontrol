/****************************************************************************
 *
 * Copyright (C) 2018 Pinecone Inc. All rights reserved.
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <jni.h>
#include <QtCore/private/qjni_p.h>
#include <QtCore/private/qjnihelpers_p.h>
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
#include "QGCApplication.h"
#include "MultiVehicleManager.h"

Q_DECLARE_LOGGING_CATEGORY(AndroidControllerLog)

class AndroidController : public QObject, public QtAndroidPrivate::KeyEventListener
{
    Q_OBJECT

public:
    AndroidController(QGCApplication* app);
    ~AndroidController();

signals:
    void cameraKeyPressed();
    void cameraKeyClicked();

private slots:
    void _activeVehicleChanged(Vehicle* activeVehicle);
    void _vehicleArmedChanged(bool armed);
    void _cameraKeyClickedHandler();
    void _handleApplicationStateChanged(Qt::ApplicationState state);

private:
    bool handleKeyEvent(jobject event);
    int ACTION_DOWN, ACTION_UP, KEYCODE_CAMERA;
    MultiVehicleManager *_multiVehicleManager;
    Vehicle *_activeVehicle;
    bool _appActive;
};
