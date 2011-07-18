#include "QGCToolWidget.h"
#include "ui_QGCToolWidget.h"

#include <QMenu>
#include <QList>
#include <QInputDialog>
#include <QDockWidget>
#include <QContextMenuEvent>
#include <QSettings>
#include <QFileDialog>
#include <QDesktopServices>

#include "QGCParamSlider.h"
#include "QGCActionButton.h"
#include "QGCCommandButton.h"
#include "UASManager.h"

QGCToolWidget::QGCToolWidget(const QString& title, QWidget *parent) :
        QWidget(parent),
        mav(NULL),
        mainMenuAction(NULL),
        ui(new Ui::QGCToolWidget)
{
    ui->setupUi(this);
    setObjectName(title);
    createActions();
    toolLayout = ui->toolLayout;
    toolLayout->setAlignment(Qt::AlignTop);
    toolLayout->setSpacing(8);

    QDockWidget* dock = dynamic_cast<QDockWidget*>(this->parentWidget());
    if (dock) {
        dock->setWindowTitle(title);
        dock->setObjectName(title+"DOCK");
    }

    // Try with parent
    dock = dynamic_cast<QDockWidget*>(parent);
    if (dock) {
        dock->setWindowTitle(title);
        dock->setObjectName(title+"DOCK");
    }

    this->setWindowTitle(title);

    QList<UASInterface*> systems = UASManager::instance()->getUASList();
    foreach (UASInterface* uas, systems) {
        UAS* newMav = dynamic_cast<UAS*>(uas);
        if (newMav) {
            addUAS(uas);
        }
    }
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addUAS(UASInterface*)));
    if (!instances()->contains(title)) instances()->insert(title, this);
}

QGCToolWidget::~QGCToolWidget()
{
    delete ui;
}

/**
 * @param parent Object later holding these widgets, usually the main window
 * @return List of all widgets
 */
QList<QGCToolWidget*> QGCToolWidget::createWidgetsFromSettings(QWidget* parent, QString settingsFile)
{
    // Load widgets from application settings
    QSettings* settings;

    // Or load them from a settings file
    if (!settingsFile.isEmpty())
    {
        settings = new QSettings(settingsFile, QSettings::IniFormat);
        qDebug() << "LOADING SETTINGS FROM" << settingsFile;
    }
    else
    {
        settings = new QSettings();
    }

    QList<QGCToolWidget*> newWidgets;
    int size = settings->beginReadArray("QGC_TOOL_WIDGET_NAMES");
    for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        QString name = settings->value("TITLE", tr("UNKNOWN WIDGET %1").arg(i)).toString();

        if (!instances()->contains(name)) {
            QGCToolWidget* tool = new QGCToolWidget(name, parent);
            instances()->insert(name, tool);
            newWidgets.append(tool);
        }
        else
        {
            qDebug() << "WIDGET DID ALREADY EXIST, REJECTING";
        }
    }
    settings->endArray();

    qDebug() << "NEW WIDGETS: " << newWidgets.size();

    // Load individual widget items
    for (int i = 0; i < newWidgets.size(); i++) {
        newWidgets.at(i)->loadSettings(*settings);
    }
    delete settings;

    return instances()->values();
}

void QGCToolWidget::loadSettings(const QString& settings)
{
    QSettings set(settings, QSettings::IniFormat);
    QStringList groups = set.childGroups();
    QString widgetName = groups.first();
    setTitle(widgetName);
    qDebug() << "WIDGET TITLE LOADED: " << widgetName;
    loadSettings(set);
}

void QGCToolWidget::loadSettings(QSettings& settings)
{
    QString widgetName = getTitle();
    settings.beginGroup(widgetName);
    int size = settings.beginReadArray("QGC_TOOL_WIDGET_ITEMS");
    qDebug() << "CHILDREN SIZE:" << size;
    for (int j = 0; j < size; j++) {
        settings.setArrayIndex(j);
        QString type = settings.value("TYPE", "UNKNOWN").toString();
        if (type != "UNKNOWN") {
            QGCToolWidgetItem* item = NULL;
            if (type == "BUTTON") {
                item = new QGCActionButton(this);
                qDebug() << "CREATED BUTTON";
            } else if (type == "COMMANDBUTTON") {
                item = new QGCCommandButton(this);
                qDebug() << "CREATED COMMANDBUTTON";
            } else if (type == "SLIDER") {
                item = new QGCParamSlider(this);
                qDebug() << "CREATED PARAM SLIDER";
            }

            if (item) {
                // Configure and add to layout
                addToolWidget(item);
                item->readSettings(settings);

                qDebug() << "Created tool widget";
            }
        } else {
            qDebug() << "UNKNOWN TOOL WIDGET TYPE";
        }
    }
    settings.endArray();
    settings.endGroup();
}

void QGCToolWidget::storeWidgetsToSettings(QString settingsFile)
{
    // Store list of widgets
    QSettings* settings;
    if (!settingsFile.isEmpty())
    {
        settings = new QSettings(settingsFile, QSettings::IniFormat);
    }
    else
    {
        settings = new QSettings();
    }

    settings->beginWriteArray("QGC_TOOL_WIDGET_NAMES");
    for (int i = 0; i < instances()->size(); ++i) {
        settings->setArrayIndex(i);
        settings->setValue("TITLE", instances()->values().at(i)->getTitle());
    }
    settings->endArray();

    // Store individual widget items
    for (int i = 0; i < instances()->size(); ++i) {
        instances()->values().at(i)->storeSettings(*settings);
    }
    delete settings;
}

void QGCToolWidget::storeSettings(const QString& settingsFile)
{
    QSettings settings(settingsFile, QSettings::IniFormat);
    storeSettings(settings);
}

void QGCToolWidget::storeSettings(QSettings& settings)
{
    QString widgetName = getTitle();
    settings.beginGroup(widgetName);
    settings.beginWriteArray("QGC_TOOL_WIDGET_ITEMS");
    int k = 0; // QGCToolItem counter
    for (int j = 0; j  < children().size(); ++j) {
        // Store only QGCToolWidgetItems
        QGCToolWidgetItem* item = dynamic_cast<QGCToolWidgetItem*>(children().at(j));
        if (item) {
            settings.setArrayIndex(k++);
            // Store the ToolWidgetItem
            item->writeSettings(settings);
        }
    }
    settings.endArray();
    settings.endGroup();
}

void QGCToolWidget::addUAS(UASInterface* uas)
{
    UAS* newMav = dynamic_cast<UAS*>(uas);
    if (newMav) {
        // FIXME Convert to list
        if (mav == NULL) mav = newMav;
    }
}

void QGCToolWidget::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.addAction(addParamAction);
    menu.addAction(addCommandAction);
    menu.addAction(setTitleAction);
    menu.addAction(exportAction);
    menu.addAction(deleteAction);
    menu.addSeparator();
    menu.addAction(addButtonAction);
    menu.exec(event->globalPos());
}

void QGCToolWidget::hideEvent(QHideEvent* event)
{
    // Store settings
    storeWidgetsToSettings();
    QWidget::hideEvent(event);
}

/**
 * The widgets current view and the applied dock widget area.
 * Both values are only stored internally and allow an external
 * widget to configure it accordingly
 */
void QGCToolWidget::setViewVisibilityAndDockWidgetArea(int view, bool visible, Qt::DockWidgetArea area)
{
    viewVisible.insert(view, visible);
    dockWidgetArea.insert(view, area);
}

void QGCToolWidget::createActions()
{
    addParamAction = new QAction(tr("New &Parameter Slider"), this);
    addParamAction->setStatusTip(tr("Add a parameter setting slider widget to the tool"));
    connect(addParamAction, SIGNAL(triggered()), this, SLOT(addParam()));

    addCommandAction = new QAction(tr("New MAV &Command Button"), this);
    addCommandAction->setStatusTip(tr("Add a new action button to the tool"));
    connect(addCommandAction, SIGNAL(triggered()), this, SLOT(addCommand()));

    setTitleAction = new QAction(tr("Set Widget Title"), this);
    setTitleAction->setStatusTip(tr("Set the title caption of this tool widget"));
    connect(setTitleAction, SIGNAL(triggered()), this, SLOT(setTitle()));

    deleteAction = new QAction(tr("Delete this widget"), this);
    deleteAction->setStatusTip(tr("Delete this widget permanently"));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteWidget()));

    exportAction = new QAction(tr("Export this widget"), this);
    exportAction->setStatusTip(tr("Export this widget to be reused by others"));
    connect(exportAction, SIGNAL(triggered()), this, SLOT(exportWidget()));

    importAction = new QAction(tr("Import widget"), this);
    importAction->setStatusTip(tr("Import this widget from a file (current content will be removed)"));
    connect(exportAction, SIGNAL(triggered()), this, SLOT(importWidget()));

    addButtonAction = new QAction(tr("New MAV Action Button (Deprecated)"), this);
    addButtonAction->setStatusTip(tr("Add a new action button to the tool"));
    connect(addButtonAction, SIGNAL(triggered()), this, SLOT(addAction()));
}

QMap<QString, QGCToolWidget*>* QGCToolWidget::instances()
{
    static QMap<QString, QGCToolWidget*>* instances;
    if (!instances) instances = new QMap<QString, QGCToolWidget*>();
    return instances;
}

QList<QGCToolWidgetItem*>* QGCToolWidget::itemList()
{
    static QList<QGCToolWidgetItem*>* instances;
    if (!instances) instances = new QList<QGCToolWidgetItem*>();
    return instances;
}

void QGCToolWidget::addParam()
{
    QGCParamSlider* slider = new QGCParamSlider(this);
    if (ui->hintLabel) {
        ui->hintLabel->deleteLater();
        ui->hintLabel = NULL;
    }
    toolLayout->addWidget(slider);
    slider->startEditMode();
}

void QGCToolWidget::addAction()
{
    QGCActionButton* button = new QGCActionButton(this);
    if (ui->hintLabel) {
        ui->hintLabel->deleteLater();
        ui->hintLabel = NULL;
    }
    toolLayout->addWidget(button);
    button->startEditMode();
}

void QGCToolWidget::addCommand()
{
    QGCCommandButton* button = new QGCCommandButton(this);
    if (ui->hintLabel) {
        ui->hintLabel->deleteLater();
        ui->hintLabel = NULL;
    }
    toolLayout->addWidget(button);
    button->startEditMode();
}

void QGCToolWidget::addToolWidget(QGCToolWidgetItem* widget)
{
    if (ui->hintLabel) {
        ui->hintLabel->deleteLater();
        ui->hintLabel = NULL;
    }
    toolLayout->addWidget(widget);
}

void QGCToolWidget::exportWidget()
{
    const QString widgetFileExtension(".qgw");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Specify File Name"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("QGroundControl Widget (*%1);;").arg(widgetFileExtension));
    if (!fileName.endsWith(widgetFileExtension))
    {
        fileName = fileName.append(widgetFileExtension);
    }
    storeSettings(fileName);
}

void QGCToolWidget::importWidget()
{
    const QString widgetFileExtension(".qgw");
    QString fileName = QFileDialog::getOpenFileName(this, tr("Specify File Name"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("QGroundControl Widget (*%1);;").arg(widgetFileExtension));
    loadSettings(fileName);
}

const QString QGCToolWidget::getTitle()
{
    QDockWidget* parent = dynamic_cast<QDockWidget*>(this->parentWidget());
    if (parent) {
        return parent->windowTitle();
    } else {
        return this->windowTitle();
    }
}


void QGCToolWidget::setTitle()
{
    QDockWidget* parent = dynamic_cast<QDockWidget*>(this->parentWidget());
    if (parent) {
        bool ok;
        QString text = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                                             tr("Widget title:"), QLineEdit::Normal,
                                             parent->windowTitle(), &ok);
        if (ok && !text.isEmpty()) {
            QSettings settings;
            settings.beginGroup(parent->windowTitle());
            settings.remove("");
            settings.endGroup();
            parent->setWindowTitle(text);
            setWindowTitle(text);

            storeWidgetsToSettings();
            emit titleChanged(text);
            if (mainMenuAction) mainMenuAction->setText(text);
        }
    }
}

void QGCToolWidget::setTitle(QString title)
{
    QDockWidget* parent = dynamic_cast<QDockWidget*>(this->parentWidget());
    if (parent) {
        QSettings settings;
        settings.beginGroup(parent->windowTitle());
        settings.remove("");
        settings.endGroup();
        parent->setWindowTitle(title);
    }
    setWindowTitle(title);

    storeWidgetsToSettings();
    emit titleChanged(title);
    if (mainMenuAction) mainMenuAction->setText(title);
}

void QGCToolWidget::setMainMenuAction(QAction* action)
{
    this->mainMenuAction = action;
}

void QGCToolWidget::deleteWidget()
{
    // Remove from settings

    // Hide
    this->hide();
    instances()->remove(getTitle());
    QSettings settings;
    settings.beginGroup(getTitle());
    settings.remove("");
    settings.endGroup();
    storeWidgetsToSettings();

    // Delete
    mainMenuAction->deleteLater();
    this->deleteLater();
}
