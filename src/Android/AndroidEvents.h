#pragma once

#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>
#include <QtCore/private/qandroidextras_p.h>

Q_DECLARE_LOGGING_CATEGORY(AndroidEventsLog)

class AndroidEvents : public QObject,
                      public QtAndroidPrivate::ResumePauseListener,
                      public QtAndroidPrivate::ActivityResultListener,
                      public QtAndroidPrivate::NewIntentListener
{
    Q_OBJECT

public:
    static AndroidEvents *instance();

    void handleResume() override;
    void handlePause() override;
    bool handleActivityResult(jint requestCode, jint resultCode, jobject data) override;
    bool handleNewIntent(JNIEnv *env, jobject intent) override;

signals:
    void resumed();
    void paused();
    void activityResult(int requestCode, int resultCode, QJniObject data);
    void newIntent(QJniObject intent);

public:
    explicit AndroidEvents(QObject *parent = nullptr);
    ~AndroidEvents() override;
};
