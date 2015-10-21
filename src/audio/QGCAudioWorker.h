#ifndef QGCAUDIOWORKER_H
#define QGCAUDIOWORKER_H

#include <QObject>
#include <QTimer>
#ifdef QGC_NOTIFY_TUNES_ENABLED
#include <QSound>
#endif

#if defined _MSC_VER && defined QGC_SPEECH_ENABLED
// Documentation: http://msdn.microsoft.com/en-us/library/ee125082%28v=VS.85%29.aspx
#include <basetyps.h>
#include <sapi.h>
#endif

class QGCAudioWorker : public QObject
{
    Q_OBJECT
public:
    explicit QGCAudioWorker(QObject *parent = 0);
    ~QGCAudioWorker();

    void mute(bool mute);
    bool isMuted();
    void init();

    static QString fixTextMessageForAudio(const QString& string);

signals:

public slots:
    /** @brief Say this text if current output priority matches */
    void say(QString text, int severity = 1);

    /** @brief Sound a single beep */
    void beep();

protected:
    int voiceIndex;   ///< The index of the flite voice to use (awb, slt, rms)
#if defined _MSC_VER && defined QGC_SPEECH_ENABLED
    ISpVoice *pVoice;
#endif
#ifdef QGC_NOTIFY_TUNES_ENABLED
    QSound *sound;
#endif
    bool emergency;   ///< Emergency status flag
    QTimer *emergencyTimer;
    bool muted;
private:
    static bool _getMillisecondString(const QString& string, QString& match, int& number);
};

#endif // QGCAUDIOWORKER_H
