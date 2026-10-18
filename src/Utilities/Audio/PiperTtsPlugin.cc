#include "PiperTtsPlugin.h"

#include "PiperTtsEngine.h"

QTextToSpeechEngine* PiperTtsPlugin::createTextToSpeechEngine(const QVariantMap& parameters, QObject* parent,
                                                              QString* errorString) const
{
    Q_UNUSED(parameters);
    Q_UNUSED(errorString);
    return new PiperTtsEngine(parent);
}
