#include <QSettings>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QRegularExpression>

#include "QGC.h"
#include "QGCAudioWorker.h"
#include "GAudioOutput.h"

#if (defined __macos__) && defined QGC_SPEECH_ENABLED
#include <ApplicationServices/ApplicationServices.h>

static SpeechChannel sc;
static Fixed volume;

static void speechDone(SpeechChannel sc2, void *) {
    if (sc2 == sc)
    {
        DisposeSpeechChannel(sc);
    }
}

class MacSpeech
{
public:
    MacSpeech()
    {
        setVolume(100);
    }
    ~MacSpeech()
    {
    }
    void say(const char* words)
    {
        while (SpeechBusy()) {
            QGC::SLEEP::msleep(100);
        }
        NewSpeechChannel(NULL, &sc);
        SetSpeechInfo(sc, soVolume, &volume);
        SetSpeechInfo(sc, soSpeechDoneCallBack, reinterpret_cast<void *>(speechDone));
        CFStringRef cfstr = CFStringCreateWithCString(NULL, words, kCFStringEncodingUTF8);
        SpeakCFString(sc, cfstr, NULL);
    }
    void setVolume(int v)
    {
        volume = FixRatio(v, 100);
    }
};

//-- Singleton
MacSpeech macSpeech;

#endif

#if (defined __ios__) && defined QGC_SPEECH_ENABLED
extern void iOSSpeak(QString msg);
#endif

// Speech synthesis is only supported with MSVC compiler
#if defined _MSC_VER && defined QGC_SPEECH_ENABLED
// Documentation: http://msdn.microsoft.com/en-us/library/ee125082%28v=VS.85%29.aspx
#include <sapi.h>
#endif

#if defined Q_OS_LINUX && !defined __android__ && defined QGC_SPEECH_ENABLED
// Using eSpeak for speech synthesis: following https://github.com/mondhs/espeak-sample/blob/master/sampleSpeak.cpp
#include <espeak/speak_lib.h>
#endif

QString QGC_GAUDIOOUTPUT_KEY = QStringLiteral("QGC_AUDIOOUTPUT_");

QGCAudioWorker::QGCAudioWorker(QObject *parent) :
    QObject(parent),
    voiceIndex(0),
    #if defined _MSC_VER && defined QGC_SPEECH_ENABLED
    pVoice(NULL),
    #endif
    #ifdef QGC_NOTIFY_TUNES_ENABLED
    sound(NULL),
    #endif
    emergency(false),
    muted(false)
{
    // Load settings
    QSettings settings;
    muted = settings.value(QGC_GAUDIOOUTPUT_KEY + QStringLiteral("muted"), muted).toBool();
}

void QGCAudioWorker::init()
{
#ifdef QGC_NOTIFY_TUNES_ENABLED
    sound = new QSound(":/res/Alert");
#endif

#if defined Q_OS_LINUX && !defined __android__ && defined QGC_SPEECH_ENABLED
    espeak_Initialize(AUDIO_OUTPUT_SYNCH_PLAYBACK, 500, NULL, 0); // initialize for playback with 500ms buffer and no options (see speak_lib.h)
    espeak_VOICE *espeak_voice = espeak_GetCurrentVoice();
    espeak_voice->languages = "en-uk"; // Default to British English
    espeak_voice->identifier = NULL; // no specific voice file specified
    espeak_voice->name = "klatt"; // espeak voice name
    espeak_voice->gender = 2; // Female
    espeak_voice->age = 0; // age not specified
    espeak_SetVoiceByProperties(espeak_voice);
#endif

#if defined _MSC_VER && defined QGC_SPEECH_ENABLED

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
}

QGCAudioWorker::~QGCAudioWorker()
{
#if defined _MSC_VER && defined QGC_SPEECH_ENABLED
    if (pVoice) {
        pVoice->Release();
        pVoice = NULL;
    }
    ::CoUninitialize();
#endif
}

void QGCAudioWorker::say(QString inText, int severity)
{
#ifdef __android__
    Q_UNUSED(inText);
    Q_UNUSED(severity);
#else
    static bool threadInit = false;
    if (!threadInit) {
        threadInit = true;
        init();
    }

    if (!muted)
    {
        QString text = fixTextMessageForAudio(inText);
        // Prepend high priority text with alert beep
        if (severity < GAudioOutput::AUDIO_SEVERITY_CRITICAL) {
            beep();
        }

#ifdef QGC_NOTIFY_TUNES_ENABLED
        // Wait for the last sound to finish
        while (!sound->isFinished()) {
            QGC::SLEEP::msleep(100);
        }
#endif

#if defined _MSC_VER && defined QGC_SPEECH_ENABLED
        HRESULT hr = pVoice->Speak(text.toStdWString().c_str(), SPF_DEFAULT, NULL);
        if (FAILED(hr)) {
            qDebug() << "Speak failed, HR:" << QString("%1").arg(hr, 0, 16);
        }
#elif defined Q_OS_LINUX && defined QGC_SPEECH_ENABLED
        // Set size of string for espeak: +1 for the null-character
        unsigned int espeak_size = strlen(text.toStdString().c_str()) + 1;
        espeak_Synth(text.toStdString().c_str(), espeak_size, 0, POS_CHARACTER, 0, espeakCHARS_AUTO, NULL, NULL);

#elif (defined __macos__) && defined QGC_SPEECH_ENABLED
        macSpeech.say(text.toStdString().c_str());
#elif (defined __ios__) && defined QGC_SPEECH_ENABLED
        iOSSpeak(text);
#else
        // Make sure there isn't an unused variable warning when speech output is disabled
        Q_UNUSED(inText);
#endif
    }
#endif // __android__
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

    if (!muted)
    {
#ifdef QGC_NOTIFY_TUNES_ENABLED
        sound->play(":/res/Alert");
#endif
    }
}

bool QGCAudioWorker::isMuted()
{
    return this->muted;
}

bool QGCAudioWorker::_getMillisecondString(const QString& string, QString& match, int& number) {
    static QRegularExpression re(QStringLiteral("([0-9]+ms)"));
    QRegularExpressionMatchIterator i = re.globalMatch(string);
    while (i.hasNext()) {
        QRegularExpressionMatch qmatch = i.next();
        if (qmatch.hasMatch()) {
            match = qmatch.captured(0);
            number = qmatch.captured(0).replace(QLatin1String("ms"), QLatin1String("")).toInt();
            return true;
        }
    }
    return false;
}

QString QGCAudioWorker::fixTextMessageForAudio(const QString& string) {
    QString match;
    QString newNumber;
    QString result = string;
    //-- Look for codified terms
    if(result.contains(QStringLiteral("ERR "), Qt::CaseInsensitive)) {
        result.replace(QLatin1String("ERR "), QLatin1String("error "), Qt::CaseInsensitive);
    }
    if(result.contains(QStringLiteral("ERR:"), Qt::CaseInsensitive)) {
        result.replace(QLatin1String("ERR:"), QLatin1String("error."), Qt::CaseInsensitive);
    }
    if(result.contains(QStringLiteral("POSCTL"), Qt::CaseInsensitive)) {
        result.replace(QLatin1String("POSCTL"), QLatin1String("Position Control"), Qt::CaseInsensitive);
    }
    if(result.contains(QStringLiteral("ALTCTL"), Qt::CaseInsensitive)) {
        result.replace(QLatin1String("ALTCTL"), QLatin1String("Altitude Control"), Qt::CaseInsensitive);
    }
    if(result.contains(QStringLiteral("AUTO_RTL"), Qt::CaseInsensitive)) {
        result.replace(QLatin1String("AUTO_RTL"), QLatin1String("auto Return To Land"), Qt::CaseInsensitive);
    } else if(result.contains(QStringLiteral("RTL"), Qt::CaseInsensitive)) {
        result.replace(QLatin1String("RTL"), QLatin1String("Return To Land"), Qt::CaseInsensitive);
    }
    if(result.contains(QStringLiteral("ACCEL "), Qt::CaseInsensitive)) {
        result.replace(QLatin1String("ACCEL "), QLatin1String("accelerometer "), Qt::CaseInsensitive);
    }
    if(result.contains(QStringLiteral("RC_MAP_MODE_SW"), Qt::CaseInsensitive)) {
        result.replace(QLatin1String("RC_MAP_MODE_SW"), QLatin1String("RC mode switch"), Qt::CaseInsensitive);
    }
    if(result.contains(QStringLiteral("REJ."), Qt::CaseInsensitive)) {
        result.replace(QLatin1String("REJ."), QLatin1String("Rejected"), Qt::CaseInsensitive);
    }
    if(result.contains(QStringLiteral("WP"), Qt::CaseInsensitive)) {
        result.replace(QLatin1String("WP"), QLatin1String("way point"), Qt::CaseInsensitive);
    }
    if(result.contains(QStringLiteral("CMD"), Qt::CaseInsensitive)) {
        result.replace(QLatin1String("CMD"), QLatin1String("command"), Qt::CaseInsensitive);
    }
    if(result.contains(QStringLiteral("COMPID"), Qt::CaseInsensitive)) {
        result.replace(QLatin1String("COMPID"), QLatin1String("component eye dee"), Qt::CaseInsensitive);
    }
    if(result.contains(QStringLiteral(" params "), Qt::CaseInsensitive)) {
        result.replace(QLatin1String(" params "), QLatin1String(" parameters "), Qt::CaseInsensitive);
    }
    if(result.contains(QStringLiteral(" id "), Qt::CaseInsensitive)) {
        result.replace(QLatin1String(" id "), QLatin1String(" eye dee "), Qt::CaseInsensitive);
    }
    int number;
    if(_getMillisecondString(string, match, number) && number > 1000) {
        if(number < 60000) {
            int seconds = number / 1000;
            newNumber = QStringLiteral("%1 second%2").arg(seconds).arg(seconds > 1 ? QChar('s') : QChar());
        } else {
            int minutes = number / 60000;
            int seconds = (number - (minutes * 60000)) / 1000;
            if (!seconds) {
                newNumber = QStringLiteral("%1 minute%2").arg(minutes).arg(minutes > 1 ? QChar('s') : QChar());
            } else {
                newNumber = QStringLiteral("%1 minute%2 and %3 second%4").arg(minutes).arg(minutes > 1 ? QChar('s') : QChar()).arg(seconds).arg(seconds > 1 ? QChar('s') : QChar());
            }
        }
        result.replace(match, newNumber);
    }
    // qDebug() << "Speech: " << result;
    return result;
}
