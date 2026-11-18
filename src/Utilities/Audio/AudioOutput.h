#pragma once

#include <chrono>

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>

class QTextToSpeech;
class Fact;
class AudioOutputTest;

class AudioOutput : public QObject
{
    Q_OBJECT

    friend AudioOutputTest;

public:
    enum class TextMod {
        None = 0,
        Translate = 1 << 0,
    };
    Q_FLAG(TextMod)
    Q_DECLARE_FLAGS(TextMods, TextMod)

    explicit AudioOutput(QObject *parent = nullptr);

    static AudioOutput *instance();

    void init(Fact *volumeFact, Fact *mutedFact);

    /// Reads the specified text with optional text modifications.
    void say(const QString &text, TextMods textMods = TextMod::None);

    /// Tests the audio output. Will stop current output before test
    void testAudioOutput();

private:
    QTextToSpeech *_engine = nullptr;
    qsizetype _textQueueSize = 0;
    bool _initialized = false;
    bool _engineSwitched = false;
    bool _speakCapable = false;
    Fact *_volumeFact = nullptr;
    Fact *_mutedFact = nullptr;
    double _lastVolume = -1.0;

    /// Returns the current volume (0.0 - 100.0) from the settings Fact.
    double _volumeSetting() const;

    /// Returns the current muted state from the settings Fact.
    bool _mutedSetting() const;

    /// Sets the TTS engine volume from the current Fact value.
    void _setVolume();

    /// Applies engine-dependent settings (locale, cached capabilities) for the current engine.
    void _applyEngineSettings();

    /// Finalizes initialization once the engine is Ready: applies settings, wires Facts, sets volume.
    void _finishInit();

    /// Returns the QTextToSpeech engine to use; empty selects Qt's default for the platform.
    static QString _engineName();

    /// Replaces a failed Piper engine with Qt's default engine for the platform.
    void _fallBackToDefaultEngine();

    static const QHash<QString, QString> _textHash;
    static const QSet<QString> _spelledAcronyms;

    static constexpr qsizetype kMaxTextQueueSize = 20;

    /// Grace period for async TTS backends (e.g. Android service binding) to reach Ready before warning.
    static constexpr std::chrono::seconds kEngineInitWarnTimeout{10};

    /// Fixes text messages for audio output.
    static QString _fixTextMessageForAudio(const QString &string);

    /// Replaces predefined abbreviations with their corresponding full forms.
    static QString _replaceAbbreviations(const QString &input);

    /// Returns the spoken form of an abbreviation token, or an empty string if none.
    /// _textHash ignores case; _spelledAcronyms matches all-caps only.
    static QString _spokenAbbreviation(const QString& token);

    /// Replaces negative signs with the word "negative".
    static QString _replaceNegativeSigns(const QString &input);

    /// Replaces decimal points with the word "point".
    static QString _replaceDecimalPoints(const QString &input);

    /// Replaces "m" (meters) with the word "meters" following numbers.
    static QString _replaceMeters(const QString &input);

    /// Converts millisecond values to a more readable format (seconds and minutes).
    static QString _convertMilliseconds(const QString &input);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(AudioOutput::TextMods)
