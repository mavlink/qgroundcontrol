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
#include "MG.h"

#include <QDebug>

#if defined Q_OS_MAC && defined QGC_SPEECH_ENABLED
#include <ApplicationServices/ApplicationServices.h>
#endif

// Speech synthesis is only supported with MSVC compiler
#if defined _MSC_VER && defined QGC_SPEECH_ENABLED
// Documentation: http://msdn.microsoft.com/en-us/library/ee125082%28v=VS.85%29.aspx
#include <sapi.h>
#endif

#if defined Q_OS_LINUX && defined QGC_SPEECH_ENABLED
// Using eSpeak for speech synthesis: following https://github.com/mondhs/espeak-sample/blob/master/sampleSpeak.cpp
#include <espeak/speak_lib.h>
#endif

#if defined _MSC_VER && defined QGC_SPEECH_ENABLED
ISpVoice *GAudioOutput::pVoice = NULL;
#endif

/**
 * This class follows the singleton design pattern
 * @see http://en.wikipedia.org/wiki/Singleton_pattern
 * A call to this function thus returns the only instance of this object
 * the call can occur at any place in the code, no reference to the
 * GAudioOutput object has to be passed.
 */
GAudioOutput *GAudioOutput::instance()
{
    static GAudioOutput *_instance = 0;

    if (_instance == 0)
    {
        _instance = new GAudioOutput();
        // Set the application as parent to ensure that this object
        // will be destroyed when the main application exits
        _instance->setParent(qApp);
    }

    return _instance;
}

#define QGC_GAUDIOOUTPUT_KEY QString("QGC_AUDIOOUTPUT_")

GAudioOutput::GAudioOutput(QObject *parent) : QObject(parent),
    voiceIndex(0),
    emergency(false),
    muted(false)
{
    // Load settings
    QSettings settings;
    muted = settings.value(QGC_GAUDIOOUTPUT_KEY + "muted", muted).toBool();


#if defined Q_OS_LINUX && defined QGC_SPEECH_ENABLED
    espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 500, NULL, 0); // initialize for playback with 500ms buffer and no options (see speak_lib.h)
    espeak_VOICE *espeak_voice = espeak_GetCurrentVoice();
    espeak_voice->languages = "en-uk"; // Default to British English
    espeak_voice->identifier = NULL; // no specific voice file specified
    espeak_voice->name = "klatt"; // espeak voice name
    espeak_voice->gender = 2; // Female
    espeak_voice->age = 0; // age not specified
    espeak_SetVoiceByProperties(espeak_voice);
#endif

#if defined _MSC_VER && defined QGC_SPEECH_ENABLED
    pVoice = NULL;

    if (FAILED(::CoInitialize(NULL)))
    {
        qDebug() << "ERROR: Creating COM object for audio output failed!";
    }

    else
    {

        HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);

        if (FAILED(hr))
        {
            qDebug() << "ERROR: Initializing voice for audio output failed!";
        }
    }

#endif

    // Prepare regular emergency signal, will be fired off on calling startEmergency()
    emergencyTimer = new QTimer();
    connect(emergencyTimer, SIGNAL(timeout()), this, SLOT(beep()));

    switch (voiceIndex)
    {
    case 0:
        selectFemaleVoice();
        break;

    default:
        selectMaleVoice();
        break;
    }
}

GAudioOutput::~GAudioOutput()
{
#if defined _MSC_VER && defined QGC_SPEECH_ENABLED
    pVoice->Release();
    pVoice = NULL;
    ::CoUninitialize();
#endif
}


void GAudioOutput::mute(bool mute)
{
    if (mute != muted)
    {
        this->muted = mute;
        QSettings settings;
        settings.setValue(QGC_GAUDIOOUTPUT_KEY + "muted", this->muted);
        emit mutedChanged(muted);
    }
}

bool GAudioOutput::isMuted()
{
    return this->muted;
}

bool GAudioOutput::say(QString text, int severity)
{
    if (!muted)
    {
        // TODO Add severity filter
        Q_UNUSED(severity);
        bool res = false;

        if (!emergency)
        {

#if defined _MSC_VER && defined QGC_SPEECH_ENABLED
            pVoice->Speak(text.toStdWString().c_str(), SPF_ASYNC, NULL);

#elif defined Q_OS_LINUX && defined QGC_SPEECH_ENABLED
            // Set size of string for espeak: +1 for the null-character
            unsigned int espeak_size = strlen(text.toStdString().c_str()) + 1;
            espeak_Synth(text.toStdString().c_str(), espeak_size, 0, POS_CHARACTER, 0, espeakCHARS_AUTO, NULL, NULL);

#elif defined Q_OS_MAC && defined QGC_SPEECH_ENABLED
            // Slashes necessary to have the right start to the sentence
            // copying data prevents SpeakString from reading additional chars
            text = "\\" + text;
            std::wstring str = text.toStdWString();
            unsigned char str2[1024] = {};
            memcpy(str2, text.toLatin1().data(), str.length());
            SpeakString(str2);
            res = true;

#else
            // Make sure there isn't an unused variable warning when speech output is disabled
            Q_UNUSED(text);
#endif
        }

        return res;
    }

    else
    {
        return false;
    }
}

/**
 * @param text This message will be played after the alert beep
 */
bool GAudioOutput::alert(QString text)
{
    if (!emergency || !muted)
    {
        // Play alert sound
        beep();
        // Say alert message
        say(text, 2);
        return true;
    }

    else
    {
        return false;
    }
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
    if (!emergency)
    {
        emergency = true;

        // Beep immediately and then start timer
        if (!muted) beep();

        emergencyTimer->start(1500);
        QTimer::singleShot(5000, this, SLOT(stopEmergency()));
    }

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
    if (emergency)
    {
        emergency = false;
        emergencyTimer->stop();
    }

    return true;
}

void GAudioOutput::beep()
{
    if (!muted)
    {
        // FIXME: Re-enable audio beeps
        // Use QFile to transform path for all OS
        //QFile f(QCoreApplication::applicationDirPath() + QString("/files/audio/alert.wav"));
        //qDebug() << "FILE:" << f.fileName();
        //m_media->setCurrentSource(Phonon::MediaSource(f.fileName().toStdString().c_str()));
        //m_media->play();
    }
}

void GAudioOutput::selectFemaleVoice()
{
#if defined Q_OS_LINUX && defined QGC_SPEECH_ENABLED
    // FIXME: Enable selecting a female voice on all platforms
    //this->voice = register_cmu_us_slt(NULL);
#endif
}

void GAudioOutput::selectMaleVoice()
{
#if defined Q_OS_LINUX && defined QGC_SPEECH_ENABLED
    // FIXME: Enable selecting a male voice on all platforms
    //this->voice = register_cmu_us_rms(NULL);
#endif
}

QStringList GAudioOutput::listVoices(void)
{
    // No voice selection is currently supported, so just return an empty list
    QStringList l;
    return l;
}
