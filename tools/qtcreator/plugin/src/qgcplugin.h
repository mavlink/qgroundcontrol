#pragma once

#include "qgcplugin_global.h"

#include <extensionsystem/iplugin.h>

namespace QGC::Internal {

class QGCPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QGCPlugin.json")

public:
    QGCPlugin();
    ~QGCPlugin() override;

    // IPlugin interface
    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

private:
    void createMenus();
    void registerWizards();
    void registerCompletionProviders();

    class QGCPluginPrivate;
    QGCPluginPrivate *d = nullptr;
};

} // namespace QGC::Internal
