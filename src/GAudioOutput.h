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
#include "QGCToolbox.h"

class QGCApplication;

/**
 * @brief Audio Output (speech synthesizer and "beep" output)
 * This class follows the singleton design pattern
 * @see http://en.wikipedia.org/wiki/Singleton_pattern
 */
class GAudioOutput : public QGCTool
{
    Q_OBJECT

public:
    GAudioOutput(QGCApplication* app);
    ~GAudioOutput();

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
    /** @brief Say this text */
    bool say(const QString& text);
    /** @brief Mute/unmute sound */
    void mute(bool mute);

signals:
    void mutedChanged(bool);
    bool textToSpeak(QString text);
    void beepOnce();

protected:
    bool muted;

#if !defined __android__
    QThread* thread;
    QGCAudioWorker* worker;
#endif

private:
    static const char* _mutedKey;
};

#endif // AUDIOOUTPUT_H

