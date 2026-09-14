#include "CustomPlugin.h"

#include "DigiviewManager.h"
#include "SVBackend.h"

#include <QtCore/QApplicationStatic>

Q_APPLICATION_STATIC(CustomPlugin, _customPluginInstance);

CustomPlugin::CustomPlugin(QObject* parent)
    : QGCCorePlugin(parent)
    , _digiviewManager(new DigiviewManager(this))
    , _backend(new SVBackend(_digiviewManager, this))
{
}

QGCCorePlugin* CustomPlugin::instance()
{
    return _customPluginInstance();
}

QString CustomPlugin::firstRunPromptResource(int id) const
{
    if (id == kSVInitialWelcomePromptId) {
        return QStringLiteral("qrc:/qml/QGroundControl/SynclairVisionUI/Flyview/SVWelcomePrompt.qml");
    }

    return QGCCorePlugin::firstRunPromptResource(id);
}
