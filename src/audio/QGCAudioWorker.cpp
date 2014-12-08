#include <QSettings>
#include <QDebug>

#include "QGC.h"
#include "QGCAudioWorker.h"

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
ISpVoice *QGCAudioWorker::pVoice = NULL;
#endif

#define QGC_GAUDIOOUTPUT_KEY QString("QGC_AUDIOOUTPUT_")

QGCAudioWorker::QGCAudioWorker(QObject *parent) :
    QObject(parent),
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
}

QGCAudioWorker::~QGCAudioWorker()
{
#if defined _MSC_VER && defined QGC_SPEECH_ENABLED
    pVoice->Release();
    pVoice = NULL;
    ::CoUninitialize();
#endif
}

void QGCAudioWorker::say(QString text, int severity)
{
    qDebug() << "TEXT" << text;
    if (!muted)
    {
        // TODO Add severity filter
        Q_UNUSED(severity);

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

            // Block the thread while busy
            // because we run in our own thread, this doesn't
            // halt the main application
            while (SpeechBusy()) {
                QGC::SLEEP::msleep(100);
            }

    #else
            // Make sure there isn't an unused variable warning when speech output is disabled
            Q_UNUSED(text);
    #endif
        }
    }
}

void QGCAudioWorker::mute(bool mute)
{
    if (mute != muted)
    {
        this->muted = mute;
        QSettings settings;
        settings.setValue(QGC_GAUDIOOUTPUT_KEY + "muted", this->muted);
//        emit mutedChanged(muted);
    }
}

void QGCAudioWorker::beep()
{
    // XXX beep beep

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

bool QGCAudioWorker::isMuted()
{
    return this->muted;
}
