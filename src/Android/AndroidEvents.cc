#include "AndroidEvents.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QApplicationStatic>

QGC_LOGGING_CATEGORY(AndroidEventsLog, "Android.AndroidEvents")

Q_APPLICATION_STATIC(AndroidEvents, _androidEvents);

AndroidEvents *AndroidEvents::instance()
{
    return _androidEvents();
}

AndroidEvents::AndroidEvents(QObject *parent)
    : QObject(parent)
{
    QtAndroidPrivate::registerResumePauseListener(this);
    QtAndroidPrivate::registerActivityResultListener(this);
    QtAndroidPrivate::registerNewIntentListener(this);
    qCDebug(AndroidEventsLog) << "Registered event listeners";
}

AndroidEvents::~AndroidEvents()
{
    QtAndroidPrivate::unregisterResumePauseListener(this);
    QtAndroidPrivate::unregisterActivityResultListener(this);
    QtAndroidPrivate::unregisterNewIntentListener(this);
    qCDebug(AndroidEventsLog) << "Unregistered event listeners";
}

void AndroidEvents::handleResume()
{
    qCDebug(AndroidEventsLog) << "App resumed";
    emit resumed();
}

void AndroidEvents::handlePause()
{
    qCDebug(AndroidEventsLog) << "App paused";
    emit paused();
}

bool AndroidEvents::handleActivityResult(jint requestCode, jint resultCode, jobject data)
{
    qCDebug(AndroidEventsLog) << "Activity result:" << requestCode << resultCode;
    emit activityResult(requestCode, resultCode, QJniObject(data));
    return true;
}

bool AndroidEvents::handleNewIntent(JNIEnv *, jobject intent)
{
    qCDebug(AndroidEventsLog) << "New intent received";
    emit newIntent(QJniObject(intent));
    return true;
}
