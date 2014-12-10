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
#include "GAudioOutput.h"
#include <QDebug>
#include <QGC.h>

GAudioOutput* GAudioOutput::_instance = NULL;
QMutex GAudioOutput::_singletonLock;

/**
 * This class follows the singleton design pattern
 * @see http://en.wikipedia.org/wiki/Singleton_pattern
 * A call to this function thus returns the only instance of this object
 * the call can occur at any place in the code, no reference to the
 * GAudioOutput object has to be passed.
 */

GAudioOutput* GAudioOutput::instance(void)
{
    if(_instance == 0) {
        _singletonLock.lock();
        if (_instance == 0) {
            _instance = new GAudioOutput(qgcApp());
            Q_CHECK_PTR(_instance);
        }
        _singletonLock.unlock();
    }
    
    Q_ASSERT(_instance);
    
    return _instance;
}

void GAudioOutput::deleteInstance(void)
{
    _instance = NULL;
    delete this;
}

GAudioOutput::GAudioOutput(QObject* parent, bool registerSingleton) :
    QGCSingleton(parent, registerSingleton),
    muted(false),
    thread(new QThread()),
    worker(new QGCAudioWorker())
{
    worker->moveToThread(thread);
    connect(this, SIGNAL(textToSpeak(QString,int)), worker, SLOT(say(QString,int)));
    connect(this, SIGNAL(beepOnce()), worker, SLOT(beep()));
    thread->start();
}

GAudioOutput::~GAudioOutput()
{
    thread->quit();
    while (thread->isRunning()) {
        QGC::SLEEP::usleep(100);
    }
    delete worker;
    delete thread;
}


void GAudioOutput::mute(bool mute)
{
    // XXX handle muting
    Q_UNUSED(mute);
}

bool GAudioOutput::isMuted()
{
    // XXX return right stuff
    return false;
}

bool GAudioOutput::say(QString text, int severity)
{
    emit textToSpeak(text, severity);
    return true;
}

/**
 * @param text This message will be played after the alert beep
 */
bool GAudioOutput::alert(QString text)
{
    emit textToSpeak(text, 1);
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
    emit beepOnce();
}
