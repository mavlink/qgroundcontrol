#include "AndroidEvents.h"

#include <QtCore/QApplicationStatic>

QGC_LOGGING_CATEGORY(AndroidEventsLog, "AndroidEvents")

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

bool AndroidEvents::handleActivityResult(int requestCode, int resultCode, const QJniObject &data)
{
    qCDebug(AndroidEventsLog) << "Activity result:" << requestCode << resultCode;
    emit activityResult(requestCode, resultCode, data);
    return true;
}

bool AndroidEvents::handleNewIntent(const QJniObject &intent)
{
    qCDebug(AndroidEventsLog) << "New intent received";
    emit newIntent(intent);
    return true;
}
