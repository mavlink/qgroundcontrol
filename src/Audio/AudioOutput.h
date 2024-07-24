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
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtTextToSpeech/QTextToSpeech>

Q_DECLARE_LOGGING_CATEGORY(AudioOutputLog)

/// The AudioOutput class provides functionality for audio output using text-to-speech.
class AudioOutput : public QTextToSpeech
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)

public:
    /// Enumeration for text modification options.
    enum class TextMod {
        None = 0,
        Translate = 1 << 0,
    };
    Q_DECLARE_FLAGS(TextMods, TextMod)
    Q_FLAG(TextMod)

    /// Constructs an AudioOutput object.
    ///     @param parent The parent QObject.
    explicit AudioOutput(QObject* parent = nullptr);

    /// Destructor for the AudioOutput class.
    ~AudioOutput();

    /// Checks if the audio output is muted.
    ///     @return True if muted, false otherwise.
    bool isMuted() const;

    /// Sets the mute state of the audio output.
    ///     @param enable True to mute, false to unmute.
    void setMuted(bool enable);

    /// Reads the specified text with optional text modifications.
    ///     @param text The text to be read.
    ///     @param textMods The text modifications to apply.
    void read(const QString& text, AudioOutput::TextMods textMods = TextMod::None);

    /// Gets the singleton instance of AudioOutput.
    ///     @return The singleton instance.
    static AudioOutput* instance();

    /// Extracts a millisecond value from the given string.
    ///     @param string The string to extract from.
    ///     @param match The extracted millisecond string.
    ///     @param number The extracted number.
    ///     @return True if extraction is successful, false otherwise.
    static bool getMillisecondString(const QString& string, QString& match, int& number);

    /// Fixes text messages for audio output.
    ///     @param string The text message to fix.
    ///     @return The fixed text message.
    static QString fixTextMessageForAudio(const QString& string);

signals:
    /// Emitted when the mute state changes.
    ///     @param muted The new mute state.
    void mutedChanged(bool muted);

private:
    qsizetype m_textQueueSize = 0;
    bool m_muted = false;

    static const QHash<QString, QString> s_textHash;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(AudioOutput::TextMods)
