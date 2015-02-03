#include "QGCToolWidget.h"
#include "ui_QGCToolWidget.h"

#include <QMenu>
#include <QList>
#include <QInputDialog>
#include <QDockWidget>
#include <QContextMenuEvent>
#include <QSettings>
#include <QStandardPaths>

#include "QGCParamSlider.h"
#include "QGCToolWidgetComboBox.h"
#include "QGCTextLabel.h"
#include "QGCXYPlot.h"
#include "QGCCommandButton.h"
#include "UASManager.h"
#include "QGCFileDialog.h"

QGCToolWidget::QGCToolWidget(const QString& objectName, const QString& title, QWidget *parent, QSettings* settings) :
        QWidget(parent),
        mav(NULL),
        mainMenuAction(NULL),
        widgetTitle(title),
        ui(new Ui::QGCToolWidget)
{
    isFromMetaData = false;
    ui->setupUi(this);
    if (settings) loadSettings(*settings);

    createActions();
    toolLayout = ui->toolLayout;
    toolLayout->setAlignment(Qt::AlignTop);
    toolLayout->setSpacing(8);

    this->setTitle(widgetTitle);
    QList<UASInterface*> systems = UASManager::instance()->getUASList();
    foreach (UASInterface* uas, systems)
    {
        UAS* newMav = dynamic_cast<UAS*>(uas);
        if (newMav)
        {
            addUAS(uas);
        }
    }
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addUAS(UASInterface*)));

    if(!objectName.isEmpty()) {
        instances()->insert(objectName, this);
        setObjectName(objectName);
    } //Otherwise we must call loadSettings() immediately to set the object name
}

QGCToolWidget::~QGCToolWidget()
{
    if (mainMenuAction) mainMenuAction->deleteLater();
    if (QGCToolWidget::instances()) QGCToolWidget::instances()->remove(objectName());
    delete ui;
}

void QGCToolWidget::setParent(QWidget *parent)
{
    QWidget::setParent(parent);
    setTitle(getTitle()); //Update titles
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
       //qDebug() << "LOADING SETTINGS FROM" << settingsFile;
    }
    else
    {
        settings = new QSettings();
        //qDebug() << "LOADING SETTINGS FROM DEFAULT" << settings->fileName();
    }

    QList<QGCToolWidget*> newWidgets;
    settings->beginGroup("Custom_Tool_Widgets");
    int size = settings->beginReadArray("QGC_TOOL_WIDGET_NAMES");
    for (int i = 0; i < size; i++)
    {
        settings->setArrayIndex(i);
        QString name = settings->value("TITLE", "").toString();
        QString objname = settings->value("OBJECT_NAME", "").toString();

        if (!instances()->contains(objname) && !objname.isEmpty())
        {
            //qDebug() << "CREATED WIDGET:" << name;
            QGCToolWidget* tool = new QGCToolWidget(objname, name, parent, settings);
            newWidgets.append(tool);
        }
        else if (name.length() == 0)
        {
            // Silently catch empty widget name - sanity check
            // to survive broken settings (e.g. from user manipulation)
        }
        else
        {
            //qDebug() << "WIDGET" << name << "DID ALREADY EXIST, REJECTING";
        }
    }
    settings->endArray();

    //qDebug() << "NEW WIDGETS: " << newWidgets.size();

    // Load individual widget items
    for (int i = 0; i < newWidgets.size(); i++)
    {
        newWidgets.at(i)->loadSettings(*settings);
    }
    settings->endGroup();
    settings->sync();
    delete settings;

    return instances()->values();
}
void QGCToolWidget::showLabel(QString name,int num)
{
    for (int i=0;i<toolItemList.size();i++)
    {
        if (toolItemList[i]->objectName() == name)
        {
            QGCTextLabel *label = qobject_cast<QGCTextLabel*>(toolItemList[i]);
            if (label)
            {
                label->enableText(num);
            }
        }
    }
}

/**
 * @param singleinstance If this is set to true, the widget settings will only be loaded if not another widget with the same title exists
 */
bool QGCToolWidget::loadSettings(const QString& settings, bool singleinstance)
{
    QSettings set(settings, QSettings::IniFormat);
    QStringList groups = set.childGroups();
    if (groups.length() > 0)
    {
        QString objectName = groups.first();
        setObjectName(objectName);
        if (singleinstance && QGCToolWidget::instances()->contains(objectName)) return false;
        instances()->insert(objectName, this);
        // Do not use setTitle() here,
        // interferes with loading settings
        widgetTitle = objectName;
        //qDebug() << "WIDGET TITLE LOADED: " << widgetName;
        loadSettings(set);
        return true;
    }
    else
    {
        return false;
    }
}
void QGCToolWidget::setSettings(QVariantMap& settings)
{
    isFromMetaData = true;
    settingsMap = settings;
    QString widgetName = getTitle();
    int size = settingsMap["count"].toInt();
    for (int j = 0; j < size; j++)
    {
        QString type = settings.value(widgetName + "\\" + QString::number(j) + "\\" + "TYPE", "UNKNOWN").toString();
        if (type == "SLIDER")
        {
            QString checkparam = settingsMap.value(widgetName + "\\" + QString::number(j) + "\\" + "QGC_PARAM_SLIDER_PARAMID").toString();
            paramList.append(checkparam);
        }
        else if (type == "COMBO")
        {
            QString checkparam = settingsMap.value(widgetName + "\\" + QString::number(j) + "\\" + "QGC_PARAM_COMBOBOX_PARAMID").toString();
            paramList.append(checkparam);
        }
    }
}
QList<QString> QGCToolWidget::getParamList()
{
    return paramList;
}
void QGCToolWidget::setParameterValue(int uas, int component, QString parameterName, const QVariant value)
{
    Q_UNUSED(uas);
    Q_UNUSED(component);
    Q_UNUSED(value);
    
    QString widgetName = getTitle();
    int size = settingsMap["count"].toInt();
    if (paramToItemMap.contains(parameterName))
    {
        //If we already have an item for this parameter, updates are handled internally.
        return;
    }

    for (int j = 0; j < size; j++)
    {
        QString type = settingsMap.value(widgetName + "\\" + QString::number(j) + "\\" + "TYPE", "UNKNOWN").toString();
        QGCToolWidgetItem* item = NULL;
        if (type == "COMMANDBUTTON")
        {
            //This shouldn't happen, but I'm not sure... so lets test for it.
            continue;
        }
        else if (type == "SLIDER")
        {
            QString checkparam = settingsMap.value(widgetName + "\\" + QString::number(j) + "\\" + "QGC_PARAM_SLIDER_PARAMID").toString();
            if (checkparam == parameterName)
            {
                item = new QGCParamSlider(this);
                paramToItemMap[parameterName] = item;
                addToolWidget(item);
                item->readSettings(widgetName + "\\" + QString::number(j) + "\\",settingsMap);
                return;
            }
        }
        else if (type == "COMBO")
        {
            QString checkparam = settingsMap.value(widgetName + "\\" + QString::number(j) + "\\" + "QGC_PARAM_COMBOBOX_PARAMID").toString();
            if (checkparam == parameterName)
            {
                item = new QGCToolWidgetComboBox(this);
                addToolWidget(item);
                item->readSettings(widgetName + "\\" + QString::number(j) + "\\",settingsMap);
                paramToItemMap[parameterName] = item;
                return;
            }
        }
    }
}

void QGCToolWidget::loadSettings(QVariantMap& settings)
{

    QString widgetName = getTitle();
    //settings.beginGroup(widgetName);
    qDebug() << "LOADING FOR" << widgetName;
    //int size = settings.beginReadArray("QGC_TOOL_WIDGET_ITEMS");
    int size = settings["count"].toInt();
    qDebug() << "CHILDREN SIZE:" << size;
    for (int j = 0; j < size; j++)
    {
        QApplication::processEvents();
        //settings.setArrayIndex(j);
        QString type = settings.value(widgetName + "\\" + QString::number(j) + "\\" + "TYPE", "UNKNOWN").toString();
        if (type != "UNKNOWN")
        {
            QGCToolWidgetItem* item = NULL;
            if (type == "COMMANDBUTTON")
            {
                item = new QGCCommandButton(this);
                //qDebug() << "CREATED COMMANDBUTTON";
            }
            else if (type == "TEXT")
            {
                item = new QGCTextLabel(this);
                item->setActiveUAS(mav);
            }
            else if (type == "SLIDER")
            {
                item = new QGCParamSlider(this);
                //qDebug() << "CREATED PARAM SLIDER";
            }
            else if (type == "COMBO")
            {
                item = new QGCToolWidgetComboBox(this);
                //qDebug() << "CREATED COMBOBOX";
            }
            else if (type == "XYPLOT")
            {
                item = new QGCXYPlot(this);
                //qDebug() << "CREATED XYPlot";
            }
            if (item)
            {
                // Configure and add to layout
                addToolWidget(item);
                item->readSettings(widgetName + "\\" + QString::number(j) + "\\",settings);

                //qDebug() << "Created tool widget";
            }
        }
        else
        {
            qDebug() << "UNKNOWN TOOL WIDGET TYPE" << type;
        }
    }
    //settings.endArray();
    //settings.endGroup();
}

void QGCToolWidget::loadSettings(QSettings& settings)
{
    QString widgetName = getTitle();
    settings.beginGroup(widgetName);
    //qDebug() << "LOADING FOR" << widgetName;
    int size = settings.beginReadArray("QGC_TOOL_WIDGET_ITEMS");
    //qDebug() << "CHILDREN SIZE:" << size;
    for (int j = 0; j < size; j++)
    {
        settings.setArrayIndex(j);
        QString type = settings.value("TYPE", "UNKNOWN").toString();
        if (type != "UNKNOWN")
        {
            QGCToolWidgetItem* item = NULL;
            if (type == "COMMANDBUTTON")
            {
                QGCCommandButton *button = new QGCCommandButton(this);
                connect(button,SIGNAL(showLabel(QString,int)),this,SLOT(showLabel(QString,int)));
                item = button;
                item->setActiveUAS(mav);
                //qDebug() << "CREATED COMMANDBUTTON";
            }
            else if (type == "SLIDER")
            {
                item = new QGCParamSlider(this);
                item->setActiveUAS(mav);
                //qDebug() << "CREATED PARAM SLIDER";
            }
            else if (type == "COMBO")
            {
                item = new QGCToolWidgetComboBox(this);
                item->setActiveUAS(mav);
                qDebug() << "CREATED PARAM COMBOBOX";
            }
            else if (type == "TEXT")
            {
                item = new QGCTextLabel(this);
                item->setObjectName(settings.value("QGC_TEXT_ID").toString());
                item->setActiveUAS(mav);
            }
            else if (type == "XYPLOT")
            {
                item = new QGCXYPlot(this);
                item->setActiveUAS(mav);
            }

            if (item)
            {
                // Configure and add to layout
                addToolWidget(item);
                item->readSettings(settings);

                //qDebug() << "Created tool widget";
            }
        }
        else
        {
            //qDebug() << "UNKNOWN TOOL WIDGET TYPE";
        }
    }
    settings.endArray();
    settings.endGroup();
}

void QGCToolWidget::storeWidgetsToSettings(QSettings &settings) //static
{
    settings.beginGroup("Custom_Tool_Widgets");
    int preArraySize = settings.beginReadArray("QGC_TOOL_WIDGET_NAMES");
    settings.endArray();

    settings.beginWriteArray("QGC_TOOL_WIDGET_NAMES");
    int num = 0;
    for (int i = 0; i < qMax(preArraySize, instances()->size()); ++i)
    {
        if (i < instances()->size())
        {
            // Updating value
            if (!instances()->values().at(i)->fromMetaData())
            {
                settings.setArrayIndex(num++);
                settings.setValue("TITLE", instances()->values().at(i)->getTitle());
                settings.setValue("OBJECT_NAME", instances()->values().at(i)->objectName());
                qDebug() << "WRITING TITLE" << instances()->values().at(i)->getTitle() << "object:" << instances()->values().at(i)->objectName();
            }
        }
        else
        {
            // Deleting old value
            settings.remove("TITLE");
        }
    }
    settings.endArray();

    // Store individual widget items
    for (int i = 0; i < instances()->size(); ++i)
    {
        instances()->values().at(i)->storeSettings(settings);
    }
    settings.endGroup();
    settings.sync();
}

void QGCToolWidget::storeSettings(QSettings& settings)
{
    /* This function should be called from storeWidgetsToSettings() which sets up the group etc */
    Q_ASSERT(settings.group() == "Custom_Tool_Widgets");

    if (isFromMetaData)
    {
        //Refuse to store if this is loaded from metadata or dynamically generated.
        return;
    }
    //qDebug() << "WRITING WIDGET" << widgetTitle << "TO SETTINGS";
    settings.beginGroup(widgetTitle);
    settings.beginWriteArray("QGC_TOOL_WIDGET_ITEMS");
    int k = 0; // QGCToolItem counter
    foreach(QGCToolWidgetItem *item, toolItemList) {
        // Only count actual tool widget item children
        settings.setArrayIndex(k++);
        // Store the ToolWidgetItem
        item->writeSettings(settings);
    }
    //qDebug() << "WROTE" << k << "SUB-WIDGETS TO SETTINGS";
    settings.endArray();
    settings.endGroup();
}

void QGCToolWidget::addUAS(UASInterface* uas)
{
    UAS* newMav = dynamic_cast<UAS*>(uas);
    if (newMav)
    {
        // FIXME Convert to list
        if (mav == NULL) mav = newMav;
    }
}

void QGCToolWidget::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.addAction(addParamAction);
    menu.addAction(addCommandAction);
    menu.addAction(addLabelAction);
    menu.addAction(addPlotAction);
    menu.addSeparator();
    menu.addAction(setTitleAction);
    menu.addAction(exportAction);
    menu.addAction(importAction);
    menu.addAction(deleteAction);
    menu.exec(event->globalPos());
}

void QGCToolWidget::hideEvent(QHideEvent* event)
{
    // Store settings
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

    addLabelAction = new QAction(tr("New &Text Label"), this);
    addLabelAction->setStatusTip(tr("Add a new label to the tool"));
    connect(addLabelAction, SIGNAL(triggered()), this, SLOT(addLabel()));

    addPlotAction = new QAction(tr("New &XY Plot"), this);
    addPlotAction->setStatusTip(tr("Add a XY Plot to the tool"));
    connect(addPlotAction, SIGNAL(triggered()), this, SLOT(addPlot()));

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
    connect(importAction, SIGNAL(triggered()), this, SLOT(importWidget()));
}

QMap<QString, QGCToolWidget*>* QGCToolWidget::instances()
{
    static QMap<QString, QGCToolWidget*>* instances;
    if (!instances) instances = new QMap<QString, QGCToolWidget*>();
    return instances;
}

void QGCToolWidget::addParam(int uas,int component,QString paramname,QVariant value)
{
    isFromMetaData = true;
    QGCParamSlider* slider = new QGCParamSlider(this);
    addToolWidget(slider);
    slider->setActiveUAS(mav);
    slider->setParameterValue(uas,component,0,-1,paramname,value);
}

void QGCToolWidget::addParam()
{
    addToolWidgetAndEdit(new QGCParamSlider(this));
}

void QGCToolWidget::addCommand()
{
    addToolWidgetAndEdit(new QGCCommandButton(this));
}

void QGCToolWidget::addLabel()
{
    addToolWidgetAndEdit(new QGCTextLabel(this));
}

void QGCToolWidget::addPlot()
{
    addToolWidgetAndEdit(new QGCXYPlot(this));
}

void QGCToolWidget::addToolWidgetAndEdit(QGCToolWidgetItem* widget)
{
    addToolWidget(widget);
    widget->startEditMode();
}

void QGCToolWidget::addToolWidget(QGCToolWidgetItem* widget)
{
    if (ui->hintLabel)
    {
        ui->hintLabel->deleteLater();
        ui->hintLabel = NULL;
    }
    connect(widget, SIGNAL(editingFinished()), this, SLOT(storeWidgetsToSettings()));
    connect(widget, SIGNAL(destroyed()), this, SLOT(widgetRemoved()));
    toolLayout->addWidget(widget);
    toolItemList.append(widget);
}

void QGCToolWidget::widgetRemoved()
{
    // Do not dynamic cast or de-reference QObject, since object is either in destructor or may have already
    // been destroyed.
    
    QGCToolWidgetItem *widget = static_cast<QGCToolWidgetItem *>(QObject::sender());
    toolItemList.removeAll(widget);
    storeWidgetsToSettings();
}

void QGCToolWidget::exportWidget()
{
    //-- Get file to save
    QString fileName = QGCFileDialog::getSaveFileName(
        this, tr("Save Widget File"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        tr("QGroundControl Widget (*.qgw)"),
        "qgw",
        true);
    //-- Save it if we have it
    if (!fileName.isEmpty()) {
        QSettings settings(fileName, QSettings::IniFormat);
        storeSettings(settings);
    }
}

void QGCToolWidget::importWidget()
{
    QString fileName = QGCFileDialog::getOpenFileName(
        this, tr("Load Widget File"), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        tr("QGroundControl Widget (*.qgw);;All Files (*)"));
    if (!fileName.isEmpty()) {
        // TODO There is no error checking. If the load fails, there is nothing telling the user what happened.
        loadSettings(fileName);
    }
}

QString QGCToolWidget::getTitle() const
{
    return widgetTitle;
}

void QGCToolWidget::setTitle()
{
    QDockWidget* parent = dynamic_cast<QDockWidget*>(this->parentWidget());
    if (parent)
    {
        bool ok;
        QString text = QInputDialog::getText(this, tr("Enter Widget Title"),
                                             tr("Widget title:"), QLineEdit::Normal,
                                             parent->windowTitle(), &ok);
        if (ok && !text.isEmpty())
        {
            setTitle(text);
        }
    }
}

void QGCToolWidget::setTitle(const QString& title)
{
    // Sets title and calls setWindowTitle on QWidget
    widgetTitle = title;
    QWidget::setWindowTitle(title);
    QDockWidget* dock = dynamic_cast<QDockWidget*>(this->parentWidget());
    if (dock)
        dock->setWindowTitle(widgetTitle);
    emit titleChanged(title);
    if (mainMenuAction) mainMenuAction->setText(title);

    //Do not save the settings here, because this function might be
    //called while loading, and thus saving here could end up clobbering
    //all of the other widgets
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
    instances()->remove(objectName());

    QSettings settings;
    settings.beginGroup("QGC_MAINWINDOW");
    settings.remove(QString("TOOL_PARENT_") + objectName());
    settings.endGroup();

    storeWidgetsToSettings();

    // Delete
    this->deleteLater();
}
