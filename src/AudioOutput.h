#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QObject>
#include <QStringList>

/* Foward declarations of speech/voice structure */
#ifdef Q_OS_MAC
	struct SpeechChannelRecord;
	typedef SpeechChannelRecord *SpeechChannel;
#else
	struct cst_voice;
#endif

class AudioOutput : public QObject
{
    Q_OBJECT
public:
    AudioOutput(QString voice="", QObject* parent=NULL);
    /** @brief List available voices */
    QStringList listVoices(void);

    public slots:
    /** @brief Say this text if current output priority matches */
    bool say(QString text, int severity);
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

protected:
#ifdef Q_OS_MAC
	SpeechChannel *voice;
#else
    cst_voice *voice;
#endif
	int voiceIndex;
};

#endif // AUDIOOUTPUT_H
