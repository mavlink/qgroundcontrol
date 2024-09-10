/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

class QTextToSpeech;
class Fact;

Q_DECLARE_LOGGING_CATEGORY(AudioOutputLog)

/// The AudioOutput class provides functionality for audio output using text-to-speech.
class AudioOutput : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)

public:
    /// Enumeration for text modification options.
    enum class TextMod {
        None = 0,
        Translate = 1 << 0,
    };
    Q_FLAG(TextMod)
    Q_DECLARE_FLAGS(TextMods, TextMod)

    /// Constructs an AudioOutput object.
    ///     @param parent The parent QObject.
    explicit AudioOutput(QObject *parent = nullptr);

    /// Destructor for the AudioOutput class.
    ~AudioOutput();

    /// Gets the singleton instance of AudioOutput.
    ///     @return The singleton instance.
    static AudioOutput *instance();

    /// Initialize the Singleton
    void init(Fact *mutedFact);

    /// Checks if the audio output is muted.
    ///     @return True if muted, false otherwise.
    bool isMuted() const { return _muted; }

    /// Sets the mute state of the audio output.
    ///     @param enable True to mute, false to unmute.
    void setMuted(bool muted) { if (muted != _muted) { _muted = muted; emit mutedChanged(_muted); } }

    /// Reads the specified text with optional text modifications.
    ///     @param text The text to be read.
    ///     @param textMods The text modifications to apply.
    void say(const QString &text, AudioOutput::TextMods textMods = TextMod::None);

    /// Extracts a millisecond value from the given string.
    ///     @param string The string to extract from.
    ///     @param match The extracted millisecond string.
    ///     @param number The extracted number.
    ///     @return True if extraction is successful, false otherwise.
    static bool getMillisecondString(const QString &string, QString &match, int &number);

    /// Fixes text messages for audio output.
    ///     @param string The text message to fix.
    ///     @return The fixed text message.
    static QString fixTextMessageForAudio(const QString &string);

signals:
    /// Emitted when the mute state changes.
    ///     @param muted The new mute state.
    void mutedChanged(bool muted);

private:
    QTextToSpeech *_engine = nullptr;
    qsizetype _textQueueSize = 0;
    bool _muted = false;
    Fact *_mutedFact = nullptr;

    static const QHash<QString, QString> _textHash;

    static constexpr qsizetype kMaxTextQueueSize = 20;

};
Q_DECLARE_OPERATORS_FOR_FLAGS(AudioOutput::TextMods)
