/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AudioOutput.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QRegularExpression>

#define MAX_TEXT_QUEUE_SIZE 20U

QGC_LOGGING_CATEGORY(AudioOutputLog, "qgc.audio.audiooutput");
// qt.speech.tts.flite
// qt.speech.tts.android

const QHash<QString, QString> AudioOutput::s_textHash = {
    { "ERR",            "error" },
    { "POSCTL",         "Position Control" },
    { "ALTCTL",         "Altitude Control" },
    { "AUTO_RTL",       "auto return to launch" },
    { "RTL",            "return To launch" },
    { "ACCEL",          "accelerometer" },
    { "RC_MAP_MODE_SW", "RC mode switch" },
    { "REJ",            "rejected" },
    { "WP",             "waypoint" },
    { "CMD",            "command" },
    { "COMPID",         "component eye dee" },
    { "params",         "parameters" },
    { "id",             "I.D." },
    { "ADSB",           "A.D.S.B." },
    { "EKF",            "E.K.F." },
    { "PREARM",         "pre arm" },
    { "PITOT",          "pee toe" },
};

Q_APPLICATION_STATIC(AudioOutput, s_audioOutput);

AudioOutput* AudioOutput::instance()
{
    return s_audioOutput();
}

AudioOutput::AudioOutput(QObject* parent)
    : QTextToSpeech(QStringLiteral("none"), parent)
{
    (void) connect(this, &QTextToSpeech::stateChanged, [](QTextToSpeech::State state) {
        qCDebug(AudioOutputLog) << Q_FUNC_INFO << "State:" << state;
    });
    (void) connect(this, &QTextToSpeech::errorOccurred, [](QTextToSpeech::ErrorReason reason, const QString &errorString) {
        qCDebug(AudioOutputLog) << Q_FUNC_INFO << "Error: (" << reason << ") " << errorString;
    });
    (void) connect(this, &QTextToSpeech::volumeChanged, [](double volume) {
        qCDebug(AudioOutputLog) << Q_FUNC_INFO << "volume:" << volume;
    });

    if (!QTextToSpeech::availableEngines().isEmpty()) {
        if (setEngine(QString())) { // Autoselect engine by priority
            qCDebug(AudioOutputLog) << Q_FUNC_INFO << "engine:" << engine();
            if (availableLocales().contains(QLocale("en_US"))) {
                setLocale(QLocale("en_US"));
            }

            (void) connect(this, &AudioOutput::mutedChanged, [ this ](bool muted) {
                qCDebug(AudioOutputLog) << Q_FUNC_INFO << "muted:" << muted;
                (void) QMetaObject::invokeMethod(this, "setVolume", Qt::AutoConnection, muted ? 0. : 1.);
            });
        }
    }
}

bool AudioOutput::isMuted() const
{
    return m_muted;
}

void AudioOutput::setMuted(bool enable)
{
    if (enable != isMuted()) {
        m_muted = enable;
        emit mutedChanged(m_muted);
    }
}

void AudioOutput::read(const QString& text, AudioOutput::TextMods textMods)
{
    if(m_muted) {
        return;
    }

    if (!engineCapabilities().testFlag(QTextToSpeech::Capability::Speak)) {
        qCWarning(AudioOutputLog) << Q_FUNC_INFO << "Speech Not Supported:" << text;
        return;
    }

    if (m_textQueueSize > MAX_TEXT_QUEUE_SIZE) {
        (void) QMetaObject::invokeMethod(this, "stop", Qt::AutoConnection, QTextToSpeech::BoundaryHint::Default);
    }

    QString outText = AudioOutput::fixTextMessageForAudio(text);

    if (textMods.testFlag(AudioOutput::TextMod::Translate)) {
        outText = QCoreApplication::translate("AudioOutput", outText.toStdString().c_str());
    }

    qsizetype index = -1;
    (void) QMetaObject::invokeMethod(this, "enqueue", Qt::AutoConnection, qReturnArg(m_textQueueSize), outText);
    if (index != -1) {
        m_textQueueSize = index;
    }
}

bool AudioOutput::getMillisecondString(const QString& string, QString& match, int& number)
{
    static const QRegularExpression regexp("([0-9]+ms)");

    bool result = false;

    for (const QRegularExpressionMatch &tempMatch : regexp.globalMatch(string)) {
        if (tempMatch.hasMatch()) {
            match = tempMatch.captured(0);
            number = tempMatch.captured(0).replace("ms", "").toInt();
            result = true;
            break;
        }
    }

    return result;
}

QString AudioOutput::fixTextMessageForAudio(const QString& string)
{
    QString result = string;

    for (const QString& word: string.split(' ', Qt::SkipEmptyParts)) {
        if (s_textHash.contains(word.toUpper())) {
            result.replace(word, s_textHash.value(word.toUpper()));
        }
    }

    // Convert negative numbers
    static const QRegularExpression negNumRegex(QStringLiteral("(-)[0-9]*\\.?[0-9]"));
    QRegularExpressionMatch negNumRegexMatch = negNumRegex.match(result);
    while (negNumRegexMatch.hasMatch()) {
        if (!negNumRegexMatch.captured(1).isNull()) {
            result.replace(negNumRegexMatch.capturedStart(1), negNumRegexMatch.capturedEnd(1) - negNumRegexMatch.capturedStart(1), tr(" negative "));
        }
        negNumRegexMatch = negNumRegex.match(result);
    }

    // Convert real number with decimal point
    static const QRegularExpression realNumRegex(QStringLiteral("([0-9]+)(\\.)([0-9]+)"));
    QRegularExpressionMatch realNumRegexMatch = realNumRegex.match(result);
    while (realNumRegexMatch.hasMatch()) {
        if (!realNumRegexMatch.captured(2).isNull()) {
            result.replace(realNumRegexMatch.capturedStart(2), realNumRegexMatch.capturedEnd(2) - realNumRegexMatch.capturedStart(2), tr(" point "));
        }
        realNumRegexMatch = realNumRegex.match(result);
    }

    // Convert meter postfix after real number
    static const QRegularExpression realNumMeterRegex(QStringLiteral("[0-9]*\\.?[0-9]\\s?(m)([^A-Za-z]|$)"));
    QRegularExpressionMatch realNumMeterRegexMatch = realNumMeterRegex.match(result);
    while (realNumMeterRegexMatch.hasMatch()) {
        if (!realNumMeterRegexMatch.captured(1).isNull()) {
            result.replace(realNumMeterRegexMatch.capturedStart(1), realNumMeterRegexMatch.capturedEnd(1) - realNumMeterRegexMatch.capturedStart(1), tr(" meters"));
        }
        realNumMeterRegexMatch = realNumMeterRegex.match(result);
    }

    QString match;
    int number;
    if (getMillisecondString(string, match, number) && (number > 1000)) {
        QString newNumber;
        if (number < 60000) {
            const int seconds = number / 1000;
            newNumber = QString("%1 second%2").arg(seconds).arg(seconds > 1 ? "s" : "");
        } else {
            const int minutes = number / 60000;
            const int seconds = (number - (minutes * 60000)) / 1000;
            if (!seconds) {
                newNumber = QString("%1 minute%2").arg(minutes).arg(minutes > 1 ? "s" : "");
            } else {
                newNumber = QString("%1 minute%2 and %3 second%4").arg(minutes).arg(minutes > 1 ? "s" : "").arg(seconds).arg(seconds > 1 ? "s" : "");
            }
        }
        result.replace(match, newNumber);
    }

    return result;
}
