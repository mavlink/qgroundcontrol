#pragma once

#include <QtCore/QObject>
#include <QtTextToSpeech/QTextToSpeechPlugin>

/// Registers PiperTtsEngine as the "piper" QTextToSpeech engine. Its priority is below speechd so it is
/// only used when requested by name.
class PiperTtsPlugin : public QObject, public QTextToSpeechPlugin
{
    Q_OBJECT
    Q_INTERFACES(QTextToSpeechPlugin)
    Q_PLUGIN_METADATA(IID "org.qt-project.qt.speech.tts.plugin/6.0" FILE "PiperTtsPlugin.json")

public:
    QTextToSpeechEngine* createTextToSpeechEngine(const QVariantMap& parameters, QObject* parent,
                                                  QString* errorString) const override;
};
