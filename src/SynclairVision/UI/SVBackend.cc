#include "SVBackend.h"

#include "CustomPlugin.h"

SVBackend::SVBackend(DigiviewManager* digiview, QObject* parent)
    : QObject(parent)
    , _digiview(digiview)
{
}

SVBackend* SVBackend::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    return static_cast<CustomPlugin*>(CustomPlugin::instance())->backend();
}
