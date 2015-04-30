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
 *   @brief Definition of audio output
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef GAUDIOOUTPUT_H
#define GAUDIOOUTPUT_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QStringList>

#include "QGCAudioWorker.h"
#include "QGCSingleton.h"

/**
 * @brief Audio Output (speech synthesizer and "beep" output)
 * This class follows the singleton design pattern
 * @see http://en.wikipedia.org/wiki/Singleton_pattern
 */
class GAudioOutput : public QGCSingleton
{
    Q_OBJECT
    
    DECLARE_QGC_SINGLETON(GAudioOutput, GAudioOutput)
    
public:
    /** @brief List available voices */
    QStringList listVoices(void);
    enum
    {
        VOICE_MALE = 0,
        VOICE_FEMALE
    } QGVoice;

    enum AUDIO_SEVERITY
    {
        AUDIO_SEVERITY_EMERGENCY = 0,
        AUDIO_SEVERITY_ALERT = 1,
        AUDIO_SEVERITY_CRITICAL = 2,
        AUDIO_SEVERITY_ERROR = 3,
        AUDIO_SEVERITY_WARNING = 4,
        AUDIO_SEVERITY_NOTICE = 5,
        AUDIO_SEVERITY_INFO = 6,
        AUDIO_SEVERITY_DEBUG = 7
    };

    /** @brief Get the mute state */
    bool isMuted();

public slots:
    /** @brief Say this text if current output priority matches */
    bool say(QString text, int severity = 6);
    /** @brief Play alert sound and say notification message */
    bool alert(QString text);
    /** @brief Play emergency sound once */
    void beep();
    /** @brief Mute/unmute sound */
    void mute(bool mute);

signals:
    void mutedChanged(bool);
    bool textToSpeak(QString text, int severity = 1);
    void beepOnce();

protected:
    bool muted;
    QThread* thread;
    QGCAudioWorker* worker;
    
private:
    GAudioOutput(QObject *parent = NULL);
    ~GAudioOutput();
    
    static const char* _mutedKey;
};

#endif // AUDIOOUTPUT_H

