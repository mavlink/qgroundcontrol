#include "qgcplugin.h"
#include "qgcconstants.h"
#include "completion/mavlinkcompletionassist.h"
#include "wizard/factgroupwizardfactory.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/iwizardfactory.h>
#include <texteditor/texteditorconstants.h>

#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>

namespace QGC::Internal {

class QGCPlugin::QGCPluginPrivate
{
public:
    FactGroupWizardFactory *factGroupWizardFactory = nullptr;
    MAVLinkCompletionProvider *mavlinkCompletionProvider = nullptr;
};

QGCPlugin::QGCPlugin()
    : d(new QGCPluginPrivate)
{
}

QGCPlugin::~QGCPlugin()
{
    delete d;
}

bool QGCPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    // Create menus
    createMenus();

    // Register wizards
    registerWizards();

    // Register completion providers
    registerCompletionProviders();

    return true;
}

void QGCPlugin::extensionsInitialized()
{
    // Called after all plugins have been initialized
    // Can access other plugins here if needed
}

ExtensionSystem::IPlugin::ShutdownFlag QGCPlugin::aboutToShutdown()
{
    // Called before Qt Creator shuts down
    // Save settings, close connections, etc.
    return SynchronousShutdown;
}

void QGCPlugin::createMenus()
{
    // Get the Tools menu
    Core::ActionContainer *toolsMenu = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);

    // Create QGC submenu
    Core::ActionContainer *qgcMenu = Core::ActionManager::createMenu(Constants::MENU_ID);
    qgcMenu->menu()->setTitle(tr("QGroundControl"));
    toolsMenu->addMenu(qgcMenu);

    // Add FactGroup Wizard action
    auto factGroupAction = new QAction(tr("New FactGroup..."), this);
    Core::Command *factGroupCmd = Core::ActionManager::registerAction(
        factGroupAction,
        Constants::ACTION_FACTGROUP_WIZARD,
        Core::Context(Core::Constants::C_GLOBAL));
    factGroupCmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+Alt+F")));
    qgcMenu->addAction(factGroupCmd);

    connect(factGroupAction, &QAction::triggered, this, []() {
        // The wizard is triggered via File > New, but we can also launch it from here
        QMessageBox::information(
            Core::ICore::dialogParent(),
            tr("FactGroup Wizard"),
            tr("Use File > New File > QGroundControl > FactGroup to create a new FactGroup.\n\n"
               "Or use the Python generator:\n"
               "python -m tools.generators.factgroup.cli --spec your_spec.yaml"));
    });

    // Add separator
    qgcMenu->addSeparator();

    // Add Null-Check action (placeholder for future)
    auto nullCheckAction = new QAction(tr("Check Vehicle Null Safety"), this);
    Core::Command *nullCheckCmd = Core::ActionManager::registerAction(
        nullCheckAction,
        Constants::ACTION_NULL_CHECK,
        Core::Context(Core::Constants::C_GLOBAL));
    qgcMenu->addAction(nullCheckCmd);

    connect(nullCheckAction, &QAction::triggered, this, []() {
        QMessageBox::information(
            Core::ICore::dialogParent(),
            tr("Null Safety Check"),
            tr("Vehicle null-safety analysis is provided by the QGC LSP server.\n\n"
               "Enable it in Edit > Preferences > Language Client\n"
               "or install the QGCTools Lua extension."));
    });
}

void QGCPlugin::registerWizards()
{
    // Register FactGroup wizard
    d->factGroupWizardFactory = new FactGroupWizardFactory;
    Core::IWizardFactory::registerFactoryCreator([]() {
        return QList<Core::IWizardFactory *>{new FactGroupWizardFactory};
    });
}

void QGCPlugin::registerCompletionProviders()
{
    // Register MAVLink completion provider for C++ files
    d->mavlinkCompletionProvider = new MAVLinkCompletionProvider(this);

    // The completion provider will be automatically picked up by TextEditor
    // when editing C++ files. Qt Creator's completion system queries all
    // registered providers and merges their results.
    //
    // Note: For more specific activation, we could use:
    // - TextEditor::TextEditorActionHandler for editor-specific providers
    // - Core::EditorManager signals for editor change events
}

} // namespace QGC::Internal
