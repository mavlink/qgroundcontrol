/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

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

IMPLEMENT_QGC_SINGLETON(GAudioOutput, GAudioOutput)

const char* GAudioOutput::_mutedKey = "AudioMuted";

GAudioOutput::GAudioOutput(QObject *parent) :
    QGCSingleton(parent),
    muted(false),
    thread(new QThread()),
    worker(new QGCAudioWorker())
{
    QSettings settings;
    
    muted = settings.value(_mutedKey, false).toBool();
    muted |= qgcApp()->runningUnitTests();
    
    worker->moveToThread(thread);
    connect(this, &GAudioOutput::textToSpeak, worker, &QGCAudioWorker::say);
    connect(this, &GAudioOutput::beepOnce, worker, &QGCAudioWorker::beep);
    thread->start();
}

GAudioOutput::~GAudioOutput()
{
    thread->quit();
    thread->wait();

    delete worker;
    delete thread;
}


void GAudioOutput::mute(bool mute)
{
    QSettings settings;
    
    muted = mute;
    settings.setValue(_mutedKey, mute);
    
    emit mutedChanged(mute);
}

bool GAudioOutput::isMuted()
{
    return muted;
}

bool GAudioOutput::say(QString text, int severity)
{
    if (!muted) {
        emit textToSpeak(text, severity);
    }
    return true;
}

/**
 * @param text This message will be played after the alert beep
 */
bool GAudioOutput::alert(QString text)
{
    if (!muted) {
        emit textToSpeak(text, 1);
    }
    return true;
}

void GAudioOutput::notifyPositive()
{
    if (!muted)
    {
        // Use QFile to transform path for all OS
        // FIXME: Get working with Qt5's QtMultimedia module
        //QFile f(QCoreApplication::applicationDirPath() + QString("/files/audio/double_notify.wav"));
        //m_media->setCurrentSource(Phonon::MediaSource(f.fileName().toStdString().c_str()));
        //m_media->play();
    }
}

void GAudioOutput::notifyNegative()
{
    if (!muted)
    {
        // Use QFile to transform path for all OS
        // FIXME: Get working with Qt5's QtMultimedia module
        //QFile f(QCoreApplication::applicationDirPath() + QString("/files/audio/flat_notify.wav"));
        //m_media->setCurrentSource(Phonon::MediaSource(f.fileName().toStdString().c_str()));
        //m_media->play();
    }
}

/**
 * The emergency sound will be played continously during the emergency.
 * call stopEmergency() to disable it again. No speech synthesis or other
 * audio output is available during the emergency.
 *
 * @return true if the emergency could be started, false else
 */
bool GAudioOutput::startEmergency()
{
//    if (!emergency)
//    {
//        emergency = true;

//        // Beep immediately and then start timer

//        emergencyTimer->start(1500);
//        QTimer::singleShot(5000, this, SLOT(stopEmergency()));
//    }

    return true;
}

/**
 * Stops the continous emergency sound. Use startEmergency() to start
 * the emergency sound.
 *
 * @return true if the emergency could be stopped, false else
 */
bool GAudioOutput::stopEmergency()
{
//    if (emergency)
//    {
//        emergency = false;
//        emergencyTimer->stop();
//    }

    return true;
}

void GAudioOutput::beep()
{
    if (!muted) {
        emit beepOnce();
    }
}
