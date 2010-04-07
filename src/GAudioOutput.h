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
#include <QStringList>
#include <flite.h>
#include <Phonon>

/* For Snow leopard and later
#ifdef Q_OS_MAC
#include <NSSpeechSynthesizer.h>
#endif
   */

/**
 * @brief Audio Output (speech synthesizer and "beep" output)
 * This class follows the singleton design pattern
 * @see http://en.wikipedia.org/wiki/Singleton_pattern
 */
class GAudioOutput : public QObject
{
    Q_OBJECT
public:
    /** @brief Get the singleton instance */
    static GAudioOutput* instance();
    /** @brief List available voices */
    QStringList listVoices(void);

public slots:
    /** @brief Say this text if current output priority matches */
    bool say(QString text, int severity=1);
    /** @brief Play alert sound */
    bool alert(QString text);
    /** @brief Start emergency sound */
    bool startEmergency();
    /** @brief Stop emergency sound */
    bool stopEmergency();
    /** @brief Select female voice */
    void selectFemaleVoice();
    /** @brief Select male voice */
    void selectMaleVoice();
    /** @brief Select neutral voice */
    void selectNeutralVoice();
    /** @brief Play emergency sound */
    void beep();

protected:
    #ifdef Q_OS_MAC
    //NSSpeechSynthesizer
#endif
    cst_voice* voice; ///< The flite voice object
    int voiceIndex;   ///< The index of the flite voice to use (awb, slt, rms)
    Phonon::MediaObject* m_media; ///< The output object for audio
    Phonon::AudioOutput* m_audioOutput;
    bool emergency;   ///< Emergency status flag
    QTimer* emergencyTimer;
private:
    GAudioOutput(QObject* parent=NULL);
};

#endif // AUDIOOUTPUT_H
