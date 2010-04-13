/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Implementation of audio output
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QApplication>
#include <QTemporaryFile>
#include "GAudioOutput.h"
#include "MG.h"

#include <QDebug>

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

// Speech synthesis is only supported with MSVC compiler
#if _MSC_VER
#include <sapi.h>
using System;
using System.Speech.Synthesis;
#endif

#ifdef Q_OS_LINUX
extern "C" {
#include <flite.h>
#include <cmu_us_awb/voxdefs.h>
    //#include <cmu_us_slt/voxdefs.h>
    //cst_voice *REGISTER_VOX(const char *voxdir);
    //void UNREGISTER_VOX(cst_voice *vox);
    //cst_voice *register_cmu_us_awb(const char *voxdir);
    //void unregister_cmu_us_awb(cst_voice *vox);
    cst_voice *register_cmu_us_slt(const char *voxdir);
    void unregister_cmu_us_slt(cst_voice *vox);
    cst_voice *register_cmu_us_rms(const char *voxdir);
    void unregister_cmu_us_rms(cst_voice *vox);
};
#endif

/**
 * This class follows the singleton design pattern
 * @see http://en.wikipedia.org/wiki/Singleton_pattern
 * A call to this function thus returns the only instance of this object
 * the call can occur at any place in the code, no reference to the
 * GAudioOutput object has to be passed.
 */
GAudioOutput* GAudioOutput::instance()
{
    static GAudioOutput* _instance = 0;
    if(_instance == 0) {
        _instance = new GAudioOutput();
        // Set the application as parent to ensure that this object
        // will be destroyed when the main application exits
        _instance->setParent(qApp);
    }
    return _instance;
}

GAudioOutput::GAudioOutput(QObject* parent) : QObject(parent),
#ifdef Q_OS_LINUX
voice(NULL),
#endif
voiceIndex(0),
emergency(false)
{
#ifdef Q_OS_LINUX
    flite_init();
#endif
    // Initialize audio output
    m_media = new Phonon::MediaObject(this);
    Phonon::AudioOutput *audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    createPath(m_media, audioOutput);

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

bool GAudioOutput::say(QString text, int severity)
{
    bool res = false;
    if (!emergency)
    {

        // Speech synthesis is only supported with MSVC compiler
#ifdef _MSC_VER
        SpeechSynthesizer synth = new SpeechSynthesizer();
        synth.SelectVoice("Microsoft Anna");
        synth.SpeakText("Hello, world!");
#endif

#ifdef Q_OS_LINUX
        QTemporaryFile file;
        file.setFileTemplate("XXXXXX.wav");
        if (file.open())
        {
            cst_wave* wav = flite_text_to_wave(text.toStdString().c_str(), this->voice);
            // file.fileName() returns the unique file name
            cst_wave_save(wav, file.fileName().toStdString().c_str(), "riff");
            m_media->setCurrentSource(Phonon::MediaSource(file.fileName().toStdString().c_str()));
            m_media->play();
            qDebug() << "Synthesized: " << text << ", tmp file:" << file.fileName().toStdString().c_str();
            res = true;
        }
#endif

#ifdef Q_OS_MAC
        // Slashes necessary to have the right start to the sentence
        // copying data prevents SpeakString from reading additional chars
        text = "\\" + text;
        QStdWString str = text.toStdWString();
        unsigned char str2[1024] = {};
        memcpy(str2, text.toAscii().data(), str.length());
        SpeakString(str2);
        qDebug() << "Synthesized: " << text.toAscii();
#endif
    }
    return res;
}

/**
 * @param text This message will be played after the alert beep
 */
bool GAudioOutput::alert(QString text)
{
    if (!emergency)
    {
        // Play alert sound
        m_media->setCurrentSource(Phonon::MediaSource(QString("alert.wav").toStdString().c_str()));
        qDebug() << "FILENAME:" << m_media->currentSource().fileName();
        qDebug() << "TYPE:" << m_media->currentSource().type();
        qDebug() << QString("alert.wav").toStdString().c_str();
        m_media->play();
        m_media->setCurrentSource(Phonon::MediaSource(QString("alert.wav").toStdString().c_str()));
        m_media->play();
        // Say alert message
        return true;//say(text, 2);
    }
    else
    {
        return false;
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
        beep();
        emergencyTimer->start(1500);
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
    // Use QFile to transform path for all OS
    QFile f(MG::DIR::getSupportFilesDirectory()+QString("/audio/alert.wav"));
    m_media->setCurrentSource(Phonon::MediaSource(f.fileName().toStdString().c_str()));
    m_media->play();
}

void GAudioOutput::selectFemaleVoice()
{
#ifdef Q_OS_LINUX
    this->voice = register_cmu_us_slt(NULL);
#endif
}

void GAudioOutput::selectMaleVoice()
{
#ifdef Q_OS_LINUX
    this->voice = register_cmu_us_rms(NULL);
#endif
}

/*
void GAudioOutput::selectNeutralVoice()
{
#ifdef Q_OS_LINUX
    this->voice = register_cmu_us_awb(NULL);
#endif
}*/

QStringList GAudioOutput::listVoices(void)
{
    QStringList l;
#ifdef Q_OS_LINUX
    cst_voice *voice;
    const cst_val *v;



    printf("Voices available: ");
    for (v=flite_voice_list; v; v=val_cdr(v))
    {
        voice = val_voice(val_car(v));
        QString s;
        s.sprintf("%s",voice->name);
        printf("%s",voice->name);
        l.append(s);
    }
    printf("\n");

#endif
    return l;

}
