#ifndef QGCAUDIOWORKER_H
#define QGCAUDIOWORKER_H

#include <QObject>
#include <QTimer>

/* For Snow leopard and later
#if defined Q_OS_MAC & defined QGC_SPEECH_ENABLED
#include <NSSpeechSynthesizer.h>
#endif
   */


#if defined _MSC_VER && defined QGC_SPEECH_ENABLED
// Documentation: http://msdn.microsoft.com/en-us/library/ee125082%28v=VS.85%29.aspx
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

signals:

public slots:
    /** @brief Say this text if current output priority matches */
    void say(QString text, int severity = 1);

protected:
#if defined Q_OS_MAC && defined QGC_SPEECH_ENABLED
    //NSSpeechSynthesizer
#endif
#if defined Q_OS_LINUX && defined QGC_SPEECH_ENABLED
    //cst_voice* voice; ///< The flite voice object
#endif
#if defined _MSC_VER && defined QGC_SPEECH_ENABLED
    static ISpVoice *pVoice;
#endif
    int voiceIndex;   ///< The index of the flite voice to use (awb, slt, rms)
    bool emergency;   ///< Emergency status flag
    QTimer *emergencyTimer;
    bool muted;
};

#endif // QGCAUDIOWORKER_H
