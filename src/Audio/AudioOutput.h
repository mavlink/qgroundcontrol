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

Q_DECLARE_LOGGING_CATEGORY( AudioOutputLog )

class AudioOutput : public QTextToSpeech
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE( "" )

    Q_PROPERTY( bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged )

public:
    enum class TextMod {
        None = 0,
        Translate = 1 << 0,
    };
    Q_DECLARE_FLAGS( TextMods, TextMod )
    Q_FLAG( TextMod )

    explicit AudioOutput( QObject* parent = nullptr );

    bool isMuted() const;
    void setMuted( bool enable );

    void read( const QString& text, AudioOutput::TextMods textMods = TextMod::None );

    static AudioOutput* instance();
    static bool getMillisecondString( const QString& string, QString& match, int& number );
    static QString fixTextMessageForAudio( const QString& string );

signals:
    void mutedChanged( bool muted );

private:
    qsizetype m_textQueueSize = 0;
    bool m_muted = false;
    static const QHash<QString, QString> s_textHash;
};
Q_DECLARE_OPERATORS_FOR_FLAGS( AudioOutput::TextMods )
