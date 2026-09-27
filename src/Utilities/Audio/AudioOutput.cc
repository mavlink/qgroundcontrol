#include "AudioOutput.h"
#include "Fact.h"
#include "AppMessages.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QApplicationStatic>
#include <QtCore/QTimer>
#include <QtTextToSpeech/QTextToSpeech>
#include <QtTextToSpeech/QVoice>

#include <algorithm>

QGC_LOGGING_CATEGORY(AudioOutputLog, "Utilities.AudioOutput");
// qt.speech.tts.flite
// qt.speech.tts.android

// Keys in both tables must not end in a digit: trailing instance digits are split off before lookup.
const QHash<QString, QString> AudioOutput::_textHash = {
    {"ERR", "error"},
    {"POSCTL", "Position Control"},
    {"ALTCTL", "Altitude Control"},
    {"AUTO_RTL", "auto return to launch"},
    {"RTL", "return To launch"},
    {"ACCEL", "accelerometer"},
    {"RC_MAP_MODE_SW", "RC mode switch"},
    {"REJ", "rejected"},
    {"WP", "waypoint"},
    {"CMD", "command"},
    {"COMPID", "component eye dee"},
    {"PARAMS", "parameters"},
    {"ID", "I.D."},
    {"ADSB", "A.D.S.B."},
    {"EKF", "E.K.F."},
    {"PREARM", "pre arm"},
    {"PITOT", "pee toe"},
    {"SERVOX_FUNCTION", "Servo X Function"},
    {"CNT", "count"},
    {"DNST", "density"},
    {"TKOFF", "takeoff"},
    {"TERRN", "terrain"},
    {"AROT", "autorotation"},
    {"TCAL", "temperature calibration"},
    {"PWR", "power"},
    {"PERF", "performance"},
    {"CFG", "config"},
    {"FBWA", "fly-by-wire A"},
};

// Matched case-sensitively so lowercase words ("rc", "ins") are left alone. CAN is omitted: it is spoken as a word.
const QSet<QString> AudioOutput::_spelledAcronyms = {
    "GPS",  "GNSS", "GCS",  "ESC",  "VTOL", "RC",   "IMU",  "AHRS", "INS",   "CPU",  "RPM",  "SD",   "USB",
    "PWM",  "PPS",  "CRC",  "ICAO", "SIH",  "HAGL", "FMU",  "UTM",  "MAV",   "FFT",  "AFS",  "AMSL", "MSL",
    "HDOP", "GPIO", "SITL", "ODID", "TCP",  "UDP",  "UART", "I2C",  "IOMCU", "FTP",  "NACK", "NED",  "RSSI",
    "OSD",  "MSP",  "DMA",  "DFU",  "DNA",  "EFI",  "ISR",  "WDG",  "NMEA",  "RTCM", "VTX",  "WMM",  "HW",
    "SW",   "FW",   "RX",   "LLH",  "IP",   "IO",   "RF",   "LQ",   "DEVID",
};

Q_APPLICATION_STATIC(AudioOutput, _audioOutput);

AudioOutput::AudioOutput(QObject *parent)
    : QObject(parent)
    // Auto-select a real engine, except under unit tests where the "none" backend avoids probing
    // system plugins (e.g. speechd) that emit critical load errors and trip the strict log check.
    , _engine(QGC::runningUnitTests()
                  ? new QTextToSpeech(QStringLiteral("none"), this)
                  : new QTextToSpeech(this))
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

    _volumeFact = volumeFact;
    _mutedFact = mutedFact;

    // Some QTextToSpeech backends (notably Android) initialize asynchronously, so finalize on Ready rather than bailing (Qt docs).
    (void) connect(_engine, &QTextToSpeech::stateChanged, this, [this](QTextToSpeech::State state) {
        if (state == QTextToSpeech::State::Ready) {
            _textQueueSize = 0;
            if (!_initialized) {
                _finishInit();
            }
        }
        qCDebug(AudioOutputLog) << "TTS State changed to:" << state;
    });

    (void) connect(_engine, &QTextToSpeech::errorOccurred, this, [this](QTextToSpeech::ErrorReason reason, const QString &errorString) {
        qCWarning(AudioOutputLog) << "TTS error occurred. Reason:" << reason << ", Message:" << errorString;
        _textQueueSize = 0;
    });

    // Decrement as each utterance leaves the queue so the counter tracks live backlog, not cumulative enqueues since drain.
    (void) connect(_engine, &QTextToSpeech::aboutToSynthesize, this, [this](qsizetype) {
        if (_textQueueSize > 0) {
            _textQueueSize--;
        }
    });

    (void) connect(_engine, &QTextToSpeech::engineChanged, this, [this](const QString &engine) {
        qCDebug(AudioOutputLog) << "TTS Engine set to:" << engine;
        _applyEngineSettings();
    });

    if (_engine->state() == QTextToSpeech::State::Ready) {
        _finishInit();
    } else {
        // Some backends (notably Android) start in Error state while binding to the system TTS
        // service, then transition to Ready. Defer and only warn if the engine never becomes usable.
        qCDebug(AudioOutputLog) << "QTextToSpeech engine not ready; deferring init. State:" << _engine->state();
        if (!QGC::runningUnitTests()) {
            // Skip under unit tests: the "none" backend never becomes Ready and the warning would trip the strict log check.
            QTimer::singleShot(kEngineInitWarnTimeout, this, [this]() {
                if (!_initialized) {
                    qCWarning(AudioOutputLog) << "No usable QTextToSpeech engine available. Engine:" << _engine->engine()
                                              << "State:" << _engine->state()
                                              << "Reason:" << _engine->errorReason() << _engine->errorString();
                }
            });
        }
    }
}

void AudioOutput::_finishInit()
{
    _applyEngineSettings();

    (void) connect(_volumeFact, &Fact::valueChanged, this, [this]() {
        _setVolume();
    });

    (void) connect(_mutedFact, &Fact::valueChanged, this, [this]() {
        _setVolume();
    });

    if (AudioOutputLog().isDebugEnabled()) {
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

void AudioOutput::_applyEngineSettings()
{
    if (_engine->state() != QTextToSpeech::State::Ready) {
        return;
    }

    const QLocale defaultLocale("en_US");
    if (_engine->availableLocales().contains(defaultLocale)) {
        _engine->setLocale(defaultLocale);
    }

    // Pin an explicit voice so output doesn't depend on the engine's per-OS default.
    const QList<QVoice> voices = _engine->availableVoices();
    if (!voices.isEmpty()) {
        _engine->setVoice(voices.constFirst());
    }

    _speakCapable = _engine->engineCapabilities().testFlag(QTextToSpeech::Capability::Speak);
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

    // Must normalize volume to 0.0 - 1.0 for QTextToSpeech
    const double normalizedVolume = volume / 100.0;
    (void) QMetaObject::invokeMethod(_engine, [this, volume, normalizedVolume]() {
        if (volume == 0.0) {
            // Prevent any queued text from being spoken once muted
            _engine->stop(QTextToSpeech::BoundaryHint::Immediate);
            _textQueueSize = 0;
        }
        _engine->setVolume(normalizedVolume);
    });
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

    if (!_speakCapable) {
        qCWarning(AudioOutputLog) << "Speech Not Supported:" << text;
        return;
    }

    QString outText = _fixTextMessageForAudio(text);

    if (textMods.testFlag(TextMod::Translate)) {
        outText = tr("%1").arg(outText);
    }

    if (outText.isEmpty()) {
        return;
    }

    // All queue/counter mutation must stay on the engine thread (where stateChanged resets it).
    (void) QMetaObject::invokeMethod(_engine, [this, outText]() {
        if (_textQueueSize >= kMaxTextQueueSize) {
            _engine->stop(QTextToSpeech::BoundaryHint::Immediate);
            _textQueueSize = 0;
            qCWarning(AudioOutputLog) << "Text queue exceeded maximum size. Stopped current speech.";
        }

        const qsizetype index = _engine->enqueue(outText);
        if (index < 0) {
            qCWarning(AudioOutputLog) << "Failed to enqueue speech. State:" << _engine->state()
                                      << "Reason:" << _engine->errorReason();
            return;
        }

        _textQueueSize++;
        qCDebug(AudioOutputLog) << "Enqueued text with index:" << index << ", Queue Size:" << _textQueueSize;
    });
}

void AudioOutput::testAudioOutput()
{
    if (!_initialized) {
        qCWarning(AudioOutputLog) << "AudioOutput not initialized. Call init() before using testAudioOutput().";
        return;
    }

    // Main-thread only (QML-invoked): mutates the engine and counter directly without marshaling.
    _engine->stop(QTextToSpeech::BoundaryHint::Immediate);
    _textQueueSize = 0;

    const QString testText = tr("Audio test. Volume is %1 percent").arg(_volumeSetting(), 0, 'f', 1);
    say(testText);
}

QString AudioOutput::_fixTextMessageForAudio(const QString &string)
{
    QString result = string;
    // Before abbreviations: spelled acronyms end in '.', which would hide the word a hyphen is attached to.
    result = _replaceNegativeSigns(result);
    result = _replaceAbbreviations(result);
    result = _replaceDecimalPoints(result);
    result = _replaceMeters(result);
    result = _convertMilliseconds(result);
    return result;
}

QString AudioOutput::_spokenAbbreviation(const QString& token)
{
    const auto it = _textHash.constFind(token.toUpper());
    if (it != _textHash.constEnd()) {
        return it.value();
    }

    if (!_spelledAcronyms.contains(token)) {
        return QString();
    }

    QString spelled;
    spelled.reserve(token.size() * 2);
    for (const QChar c : token) {
        spelled.append(c);
        spelled.append(QLatin1Char('.'));
    }
    return spelled;
}

QString AudioOutput::_replaceAbbreviations(const QString &input)
{
    // Captures a whole identifier-like token, splitting off trailing instance digits ("EKF3" -> "EKF", "3").
    static const QRegularExpression tokenRegex(
        QStringLiteral("(?<![A-Za-z0-9_])([A-Za-z][A-Za-z0-9_]*?)([0-9]*)(?![A-Za-z0-9_])"));

    QString output;
    output.reserve(input.size());
    qsizetype last = 0;

    QRegularExpressionMatchIterator it = tokenRegex.globalMatch(input);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString spoken = _spokenAbbreviation(match.captured(1));
        if (spoken.isEmpty()) {
            continue;
        }

        output.append(QStringView(input).mid(last, match.capturedStart() - last));
        output.append(spoken);
        if (match.capturedLength(2) > 0) {
            output.append(QLatin1Char(' '));
            output.append(match.capturedView(2));
        }
        last = match.capturedEnd();
    }
    output.append(QStringView(input).mid(last));

    return output;
}

QString AudioOutput::_replaceNegativeSigns(const QString &input)
{
    static const QRegularExpression negNumRegex(QStringLiteral("(?<![A-Za-z0-9_])-\\s*(?=\\d)"));

    QString output = input;
    (void) output.replace(negNumRegex, "negative ");
    return output;
}

QString AudioOutput::_replaceDecimalPoints(const QString &input)
{
    static const QRegularExpression realNumRegex(QStringLiteral("([0-9]+)(\\.)([0-9]+)"));

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
