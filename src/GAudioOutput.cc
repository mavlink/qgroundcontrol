/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Implementation of audio output
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Thomas Gubler <thomasgubler@gmail.com>
 *
 */

#include <QApplication>
#include <QSettings>
#include <QDebug>

#include "GAudioOutput.h"
#include "QGCApplication.h"
#include "QGC.h"

#if defined __android__
#include <QtAndroidExtras/QtAndroidExtras>
#include <QtAndroidExtras/QAndroidJniObject>
#endif

const char* GAudioOutput::_mutedKey = "AudioMuted";

GAudioOutput::GAudioOutput(QGCApplication* app)
    : QGCTool(app)
    , muted(false)
#ifndef __android__
    , thread(new QThread())
    , worker(new QGCAudioWorker())
#endif
{
    QSettings settings;
    muted = settings.value(_mutedKey, false).toBool();
    muted |= app->runningUnitTests();
#ifndef __android__
    worker->moveToThread(thread);
    connect(this, &GAudioOutput::textToSpeak, worker, &QGCAudioWorker::say);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    thread->start();
#endif
}

GAudioOutput::~GAudioOutput()
{
#ifndef __android__
    thread->quit();
#endif
}


void GAudioOutput::mute(bool mute)
{
    QSettings settings;
    muted = mute;
    settings.setValue(_mutedKey, mute);
#ifndef __android__
    emit mutedChanged(mute);
#endif
}

bool GAudioOutput::isMuted()
{
    return muted;
}

bool GAudioOutput::say(const QString& inText)
{
    if (!muted && !qgcApp()->runningUnitTests()) {
#if defined __android__
#if defined QGC_SPEECH_ENABLED
        static const char V_jniClassName[] {"org/qgroundcontrol/qgchelper/UsbDeviceJNI"};
        QAndroidJniEnvironment env;
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        QString text = QGCAudioWorker::fixTextMessageForAudio(inText);
        QAndroidJniObject javaMessage = QAndroidJniObject::fromString(text);
        QAndroidJniObject::callStaticMethod<void>(V_jniClassName, "say", "(Ljava/lang/String;)V", javaMessage.object<jstring>());
#endif
#else
        emit textToSpeak(inText);
#endif
    }
    return true;
}
