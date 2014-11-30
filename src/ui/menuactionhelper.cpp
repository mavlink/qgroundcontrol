#include "menuactionhelper.h"

MenuActionHelper::MenuActionHelper(QObject *parent) : QObject(parent),
    m_isAdvancedMode(false),
    m_dockWidgetTitleBarsEnabled(true),
    m_addedCustomSeperator(false)
{
}

QAction *MenuActionHelper::createToolAction(const QString &title, const QString &name)
{
    QAction *action = m_menuToDockNameMap.key(name);  //For sanity, check that the action is not NULL
    if(action) {
            qWarning() << "createToolAction was called for action" << name << "which already exists in the menu";
            return action;
    }

    action = new QAction(title, NULL);
    action->setCheckable(true);
    connect(action,SIGNAL(triggered(bool)),this,SLOT(showTool(bool)));
    m_menuToDockNameMap[action] = name;
    m_menu->addAction(action);
    return action;
}

void MenuActionHelper::removeDockWidget()
{
    // Do not dynamic cast or de-reference QObject, since object is either in destructor or may have already
    // been destroyed.

    QObject *dockWidget = QObject::sender();
    Q_ASSERT(dockWidget);

    qDebug() << "Dockwidget:"  << dockWidget->objectName() << "of type" << dockWidget->metaObject()->className();

    QAction *action = m_menuToDockNameMap.key(dockWidget->objectName());
    if(action) {
        m_menuToDockNameMap.remove(action);
        action->deleteLater();
    }
    QMap<MainWindow::VIEW_SECTIONS,QMap<QString,QDockWidget*> >::iterator it;
    for (it = m_centralWidgetToDockWidgetsMap.begin(); it != m_centralWidgetToDockWidgetsMap.end(); ++it) {
        QMap<QString,QDockWidget*>::iterator it2 = it.value().begin();
        while( it2 != it.value().end()) {
            if(it2.value() == dockWidget)
                it2 = it.value().erase(it2);
            else
                ++it2;
        }
    }
    //Don't delete the dockWidget because this could have been called from the dockWidget destructor
    m_dockWidgets.removeAll(static_cast<QDockWidget*>(dockWidget));
}

QAction *MenuActionHelper::createToolActionForCustomDockWidget(const QString &title, const QString& name, QDockWidget* dockWidget, MainWindow::VIEW_SECTIONS view) {
    bool found = false;
    QAction *action = NULL;
    foreach(QAction *act, m_menuToDockNameMap.keys()) {
        if(act->text() == title) {
            found = true;
            action = act;
        }
    }

    if(!found)
        action = createToolAction(title, name);
    else
        m_menuToDockNameMap[action] = name;

    m_centralWidgetToDockWidgetsMap[view][name] = dockWidget;
    connect(dockWidget, SIGNAL(destroyed()), SLOT(removeDockWidget()),Qt::UniqueConnection); //Use UniqueConnection since we might have already created this connection in createDockWidget
    connect(dockWidget, SIGNAL(visibilityChanged(bool)), action, SLOT(setChecked(bool)));
    action->setChecked(dockWidget->isVisible());
    return action;
}

QDockWidget* MenuActionHelper::createDockWidget(const QString& title,const QString& name)
{
    QDockWidget *dockWidget = new QDockWidget(title);
    m_dockWidgets.append(dockWidget);
    setDockWidgetTitleBar(dockWidget);
    dockWidget->setObjectName(name);
    connect(dockWidget, SIGNAL(destroyed()), SLOT(removeDockWidget()));

    return dockWidget;
}

bool MenuActionHelper::containsDockWidget(MainWindow::VIEW_SECTIONS view, const QString &name) const {

    return m_centralWidgetToDockWidgetsMap.contains(view) && m_centralWidgetToDockWidgetsMap[view].contains(name);
}

QDockWidget *MenuActionHelper::getDockWidget(MainWindow::VIEW_SECTIONS view, const QString &name) const {
    if(!m_centralWidgetToDockWidgetsMap.contains(view))
        return NULL;
    return m_centralWidgetToDockWidgetsMap[view].value(name);
}

void MenuActionHelper::showTool(bool show) {
    //Called when a menu item is clicked on, regardless of view.
    QAction* act = qobject_cast<QAction *>(sender());
    Q_ASSERT(act);
    if (m_menuToDockNameMap.contains(act)) {
        QString name = m_menuToDockNameMap[act];
        emit needToShowDockWidget(name, show);
    }
}

void MenuActionHelper::setDockWidgetTitleBarsEnabled(bool enabled)
{
    m_dockWidgetTitleBarsEnabled = enabled;
    for (int i = 0; i < m_dockWidgets.size(); i++)
        setDockWidgetTitleBar(m_dockWidgets[i]);
}


void MenuActionHelper::setAdvancedMode(bool advancedMode)
{
    m_isAdvancedMode = advancedMode;
    for (int i = 0; i < m_dockWidgets.size(); i++)
        setDockWidgetTitleBar(m_dockWidgets[i]);
}

void MenuActionHelper::setDockWidgetTitleBar(QDockWidget* widget)
{
    Q_ASSERT(widget);
    QWidget* oldTitleBar = widget->titleBarWidget();

    // In advanced mode, we use the default titlebar provided by Qt.
    if (m_isAdvancedMode)
    {
        widget->setTitleBarWidget(0);
    }
    // Otherwise, if just a textlabel should be shown, make that the titlebar.
    else if (m_dockWidgetTitleBarsEnabled)
    {
        QLabel* label = new QLabel(widget);
        label->setText(widget->windowTitle());
        label->installEventFilter(this); //Ignore mouse clicks
        widget->installEventFilter(this); //Update label if window title changes. See eventFilter below
        widget->setTitleBarWidget(label);
    }
    // And if nothing should be shown, use an empty widget.
    else
    {
        QWidget* newTitleBar = new QWidget(widget);
        widget->setTitleBarWidget(newTitleBar);
    }

    // Be sure to clean up the old titlebar. When using QDockWidget::setTitleBarWidget(),
    // it doesn't delete the old titlebar object.
    delete oldTitleBar;
}

bool MenuActionHelper::eventFilter(QObject *object,QEvent *event)
{
    if (event->type() == QEvent::WindowTitleChange)
    {
        QDockWidget *dock = qobject_cast<QDockWidget *>(object);
        if(dock) {
            // Update the dock title bar label
            QLabel *label = dynamic_cast<QLabel *>(dock->titleBarWidget());
            if(label)
                label->setText(dock->windowTitle());
            // Now update the action label
            QString oldObjectName = dock->objectName();
            QAction *action = m_menuToDockNameMap.key(oldObjectName);
            if(action)
                action->setText(dock->windowTitle());
            //Now modify the object name - it is a strange naming scheme..
        }
    } else if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)
    {
        if(qobject_cast<QLabel *>(object))
            return true;
    }
    return QObject::eventFilter(object,event);
}
