/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AudioOutput.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QRegularExpression>
#include <QtCore/qapplicationstatic.h>
#include <QtTextToSpeech/QTextToSpeech>

QGC_LOGGING_CATEGORY(AudioOutputLog, "qgc.audio.audiooutput");
// qt.speech.tts.flite
// qt.speech.tts.android

const QHash<QString, QString> AudioOutput::_textHash = {
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
    { "PARAMS",         "parameters" },
    { "ID",             "I.D." },
    { "ADSB",           "A.D.S.B." },
    { "EKF",            "E.K.F." },
    { "PREARM",         "pre arm" },
    { "PITOT",          "pee toe" },
    { "SERVOX_FUNCTION","Servo X Function" },
};

Q_APPLICATION_STATIC(AudioOutput, _audioOutput);

AudioOutput::AudioOutput(QObject *parent)
    : QObject(parent)
    , _engine(new QTextToSpeech(QStringLiteral("none"), this))
{
    // qCDebug(AudioOutputLog) << Q_FUNC_INFO << this;
}

AudioOutput::~AudioOutput()
{
    // qCDebug(AudioOutputLog) << Q_FUNC_INFO << this;
}

AudioOutput *AudioOutput::instance()
{
    return _audioOutput();
}

void AudioOutput::init(Fact *mutedFact)
{
    Q_CHECK_PTR(mutedFact);

    if (_initialized) {
        return;
    }

    if (QTextToSpeech::availableEngines().isEmpty()) {
        qCWarning(AudioOutputLog) << "No available QTextToSpeech engines found.";
        return;
    }

    // Autoselect engine by priority
    if (!_engine->setEngine(QString())) {
        qCWarning(AudioOutputLog) << "Failed to set the TTS engine.";
        return;
    }

    (void) connect(_engine, &QTextToSpeech::engineChanged, [this](const QString &engine) {
        qCDebug(AudioOutputLog) << "TTS Engine set to:" << engine;
        const QLocale defaultLocale = QLocale("en_US");
        if (_engine->availableLocales().contains(defaultLocale)) {
            _engine->setLocale(defaultLocale);
        }
    });

    (void) connect(_engine, &QTextToSpeech::aboutToSynthesize, [this](qsizetype id) {
        qCDebug(AudioOutputLog) << "TTS About To Synthesize ID:" << id;
        _textQueueSize--;
        qCDebug(AudioOutputLog) << "Queue Size:" << _textQueueSize;
    });

    (void) connect(mutedFact, &Fact::valueChanged, this, [this](QVariant value) {
        setMuted(value.toBool());
    });

    if (AudioOutputLog().isDebugEnabled()) {
        (void) connect(_engine, &QTextToSpeech::stateChanged, [this](QTextToSpeech::State state) {
            qCDebug(AudioOutputLog) << "TTS State changed to:" << state;
        });
        (void) connect(_engine, &QTextToSpeech::errorOccurred, [](QTextToSpeech::ErrorReason reason, const QString &errorString) {
            qCDebug(AudioOutputLog) << "TTS Error occurred. Reason:" << reason << ", Message:" << errorString;
        });
        (void) connect(_engine, &QTextToSpeech::localeChanged, [](const QLocale &locale) {
            qCDebug(AudioOutputLog) << "TTS Locale change to:" << locale;
        });
        (void) connect(_engine, &QTextToSpeech::volumeChanged, [](double volume) {
            qCDebug(AudioOutputLog) << "TTS Volume changed to:" << volume;
        });
        (void) connect(_engine, &QTextToSpeech::sayingWord, [](const QString &word, qsizetype id, qsizetype start, qsizetype length) {
            qCDebug(AudioOutputLog) << "TTS Saying:" << word << "ID:" << id << "Start:" << start << "Length:" << length;
        });
    }

    setMuted(mutedFact->rawValue().toBool());
    _initialized = true;

    qCDebug(AudioOutputLog) << "AudioOutput initialized with muted state:" << _muted;
}

void AudioOutput::setMuted(bool muted)
{
    if (_muted.exchange(muted) != muted) {
        (void) QMetaObject::invokeMethod(_engine, "setVolume", Qt::AutoConnection, muted ? 0.0 : 1.0);
        qCDebug(AudioOutputLog) << "AudioOutput muted state set to:" << muted;
    }
}

void AudioOutput::say(const QString &text, TextMods textMods)
{
    if (!_initialized) {
        qCWarning(AudioOutputLog) << "AudioOutput not initialized. Call init() before using say().";
        return;
    }

    if (_muted) {
        return;
    }

    if (!_engine->engineCapabilities().testFlag(QTextToSpeech::Capability::Speak)) {
        qCWarning(AudioOutputLog) << "Speech Not Supported:" << text;
        return;
    }

    if (_textQueueSize >= kMaxTextQueueSize) {
        (void) QMetaObject::invokeMethod(_engine, "stop", Qt::AutoConnection, QTextToSpeech::BoundaryHint::Default);
        _textQueueSize = 0;
        qCWarning(AudioOutputLog) << "Text queue exceeded maximum size. Stopped current speech.";
    }

    QString outText = _fixTextMessageForAudio(text);

    if (textMods.testFlag(TextMod::Translate)) {
        outText = tr("%1").arg(outText);
    }

    qsizetype index;
    if (QMetaObject::invokeMethod(_engine, "enqueue", Qt::AutoConnection, qReturnArg(index), outText)) {
        if (index != -1) {
            _textQueueSize++;
            qCDebug(AudioOutputLog) << "Enqueued text with index:" << index << ", Queue Size:" << _textQueueSize;
        }
    } else {
        qCWarning(AudioOutputLog) << "Failed to invoke Enqueue method.";
    }
}

QString AudioOutput::_fixTextMessageForAudio(const QString &string)
{
    QString result = string;
    result = _replaceAbbreviations(result);
    result = _replaceNegativeSigns(result);
    result = _replaceDecimalPoints(result);
    result = _replaceMeters(result);
    result = _convertMilliseconds(result);
    return result;
}

QString AudioOutput::_replaceAbbreviations(const QString &input)
{
    QString output = input;

    const QStringList wordList = input.split(' ', Qt::SkipEmptyParts);
    for (const QString &word : wordList) {
        const QString upperWord = word.toUpper();
        if (_textHash.contains(upperWord)) {
            (void) output.replace(word, _textHash.value(upperWord));
        }
    }

    return output;
}

QString AudioOutput::_replaceNegativeSigns(const QString &input)
{
    static const QRegularExpression negNumRegex(QStringLiteral("-\\s*(?=\\d)"));
    Q_ASSERT(negNumRegex.isValid());

    QString output = input;
    (void) output.replace(negNumRegex, "negative ");
    return output;
}

QString AudioOutput::_replaceDecimalPoints(const QString &input)
{
    static const QRegularExpression realNumRegex(QStringLiteral("([0-9]+)(\\.)([0-9]+)"));
    Q_ASSERT(realNumRegex.isValid());

    QString output = input;
    QRegularExpressionMatch realNumRegexMatch = realNumRegex.match(output);
    while (realNumRegexMatch.hasMatch()) {
        if (!realNumRegexMatch.captured(2).isNull()) {
            (void) output.replace(realNumRegexMatch.capturedStart(2), realNumRegexMatch.capturedEnd(2) - realNumRegexMatch.capturedStart(2), QStringLiteral(" point "));
        }
        realNumRegexMatch = realNumRegex.match(output);
    }

    return output;
}

QString AudioOutput::_replaceMeters(const QString &input)
{
    static const QRegularExpression realNumMeterRegex(QStringLiteral("[0-9]*\\.?[0-9]\\s?(m)([^A-Za-z]|$)"));
    Q_ASSERT(realNumMeterRegex.isValid());

    QString output = input;
    QRegularExpressionMatch realNumMeterRegexMatch = realNumMeterRegex.match(output);
    while (realNumMeterRegexMatch.hasMatch()) {
        if (!realNumMeterRegexMatch.captured(1).isNull()) {
            (void) output.replace(realNumMeterRegexMatch.capturedStart(1), realNumMeterRegexMatch.capturedEnd(1) - realNumMeterRegexMatch.capturedStart(1), QStringLiteral(" meters"));
        }
        realNumMeterRegexMatch = realNumMeterRegex.match(output);
    }

    return output;
}

QString AudioOutput::_convertMilliseconds(const QString &input)
{
    QString result = input;

    QString match;
    int number;
    if (_getMillisecondString(input, match, number) && (number >= 1000)) {
        QString newNumber;
        if (number < 60000) {
            const int seconds = number / 1000;
            const int ms = number - (seconds * 1000);
            newNumber = QStringLiteral("%1 second%2").arg(seconds).arg(seconds > 1 ? "s" : "");
            if (ms > 0) {
                (void) newNumber.append(QStringLiteral(" and %1 millisecond").arg(ms));
            }
        } else {
            const int minutes = number / 60000;
            const int seconds = (number - (minutes * 60000)) / 1000;
            newNumber = QStringLiteral("%1 minute%2").arg(minutes).arg(minutes > 1 ? "s" : "");
            if (seconds > 0) {
                (void) newNumber.append(QStringLiteral(" and %1 second%2").arg(seconds).arg(seconds > 1 ? "s" : ""));
            }
        }
        (void) result.replace(match, newNumber);
    }

    return result;
}

bool AudioOutput::_getMillisecondString(const QString &string, QString &match, int &number)
{
    static const QRegularExpression msRegex("((?<number>[0-9]+)ms)");
    Q_ASSERT(msRegex.isValid());

    bool result = false;

    QRegularExpressionMatch regexpMatch = msRegex.match(string);
    if (regexpMatch.hasMatch()) {
        match = regexpMatch.captured(0);
        const QString numberStr = regexpMatch.captured("number");
        number = numberStr.toInt();
        result = true;
    }

    return result;
}
