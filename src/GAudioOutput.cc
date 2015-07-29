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
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    thread->start();
}

GAudioOutput::~GAudioOutput()
{
    thread->quit();
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

void GAudioOutput::beep()
{
    if (!muted) {
        emit beepOnce();
    }
}
