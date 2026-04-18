#include "AudioOutput.h"
#include "Fact.h"
#include "QGC.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QApplicationStatic>
#include <QtTextToSpeech/QTextToSpeech>

#include <algorithm>

QGC_LOGGING_CATEGORY(AudioOutputLog, "Utilities.AudioOutput");
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
    // qCDebug(AudioOutputLog) << this;
}

AudioOutput::~AudioOutput()
{
    // qCDebug(AudioOutputLog) << this;
}

AudioOutput *AudioOutput::instance()
{
    return _audioOutput();
}

void AudioOutput::init(Fact* volumeFact, Fact* mutedFact)
{
    Q_CHECK_PTR(volumeFact);
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

    (void) connect(_engine, &QTextToSpeech::engineChanged, this, [this](const QString &engine) {
        qCDebug(AudioOutputLog) << "TTS Engine set to:" << engine;
        const QLocale defaultLocale = QLocale("en_US");
        if (_engine->availableLocales().contains(defaultLocale)) {
            _engine->setLocale(defaultLocale);
        }
    });

    (void) connect(_engine, &QTextToSpeech::aboutToSynthesize, this, [this](qsizetype id) {
        qCDebug(AudioOutputLog) << "TTS About To Synthesize ID:" << id;
        _textQueueSize--;
        qCDebug(AudioOutputLog) << "Queue Size:" << _textQueueSize;
    });

    _volumeFact = volumeFact;
    _mutedFact = mutedFact;

    (void) connect(_volumeFact, &Fact::valueChanged, this, [this]() {
        _setVolume();
    });

    (void) connect(_mutedFact, &Fact::valueChanged, this, [this]() {
        _setVolume();
    });

    if (AudioOutputLog().isDebugEnabled()) {
        (void) connect(_engine, &QTextToSpeech::stateChanged, this, [](QTextToSpeech::State state) {
            qCDebug(AudioOutputLog) << "TTS State changed to:" << state;
        });
        (void) connect(_engine, &QTextToSpeech::errorOccurred, this, [](QTextToSpeech::ErrorReason reason, const QString &errorString) {
            qCDebug(AudioOutputLog) << "TTS Error occurred. Reason:" << reason << ", Message:" << errorString;
        });
        (void) connect(_engine, &QTextToSpeech::localeChanged, this, [](const QLocale &locale) {
            qCDebug(AudioOutputLog) << "TTS Locale change to:" << locale;
        });
        (void) connect(_engine, &QTextToSpeech::volumeChanged, this, [](double volume) {
            qCDebug(AudioOutputLog) << "TTS Volume changed to:" << volume;
        });
        (void) connect(_engine, &QTextToSpeech::sayingWord, this, [](const QString &word, qsizetype id, qsizetype start, qsizetype length) {
            qCDebug(AudioOutputLog) << "TTS Saying:" << word << "ID:" << id << "Start:" << start << "Length:" << length;
        });
    }

    _initialized = true;
    _setVolume();

    qCDebug(AudioOutputLog) << "AudioOutput initialized with volume:" << _volumeSetting() << "%";
}

double AudioOutput::_volumeSetting() const
{
    return std::clamp(_volumeFact->rawValue().toDouble(), 0.0, 100.0);
}

bool AudioOutput::_mutedSetting() const
{
    return _mutedFact->rawValue().toBool();
}

void AudioOutput::_setVolume()
{
    const bool muted = _mutedSetting();
    const double volume = muted ? 0.0 : _volumeSetting();

    // qFuzzyCompare fails near zero; adding 1.0 shifts values into a safe range
    if (qFuzzyCompare(1.0 + volume, 1.0 + _lastVolume)) {
        return;
    }
    _lastVolume = volume;

    if (volume == 0.0) {
        // Prevent any queued text from being spoken once muted
        (void) QMetaObject::invokeMethod(_engine, "stop", Qt::AutoConnection, QTextToSpeech::BoundaryHint::Default);
        _textQueueSize = 0;
    }

    // Must normalize volume to 0.0 - 1.0 for QTextToSpeech
    const double normalizedVolume = volume / 100.0;
    (void) QMetaObject::invokeMethod(_engine, "setVolume", Qt::AutoConnection, normalizedVolume);
    qCDebug(AudioOutputLog) << "AudioOutput volume set to:" << volume << "%";
}

void AudioOutput::say(const QString &text, TextMods textMods)
{
    if (!_initialized) {
        if (!QGC::runningUnitTests()) {
            qCWarning(AudioOutputLog) << "AudioOutput not initialized. Call init() before using say().";
        }
        return;
    }

    if (_volumeSetting() <= 0.0 || _mutedSetting()) {
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

void AudioOutput::testAudioOutput()
{
    if (!_initialized) {
        qCWarning(AudioOutputLog) << "AudioOutput not initialized. Call init() before using testAudioOutput().";
        return;
    }

    (void) QMetaObject::invokeMethod(_engine, "stop", Qt::AutoConnection, QTextToSpeech::BoundaryHint::Default);
    _textQueueSize = 0;

    const QString testText = tr("Audio test. Volume is %1 percent").arg(_volumeSetting(), 0, 'f', 1);
    say(testText);
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
