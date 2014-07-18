#ifndef MENUACTIONHELPER_H
#define MENUACTIONHELPER_H

#include "MainWindow.h"
#include <QDockWidget>

class MenuActionHelper : public QObject
{
    Q_OBJECT
public:
    MenuActionHelper(QObject *parent = NULL);
    ~MenuActionHelper() {}

    /** @brief Get title bar mode setting */
    bool dockWidgetTitleBarsEnabled() const { return m_dockWidgetTitleBarsEnabled; }
    void setDockWidgetTitleBarsEnabled(bool enabled);
    bool isAdvancedMode() const { return m_isAdvancedMode; }
    void setAdvancedMode(bool advancedMode);
    QAction *createToolAction(const QString &title, const QString &name = QString());
    QAction *createToolActionForCustomDockWidget(const QString& title, const QString& name, QDockWidget* dockWidget, MainWindow::VIEW_SECTIONS view);
    QDockWidget *createDockWidget(const QString& title, const QString& name);
    bool containsDockWidget(MainWindow::VIEW_SECTIONS view, const QString &name) const;
    QDockWidget *getDockWidget(MainWindow::VIEW_SECTIONS view, const QString &name) const;

    /** QMenu to add QActions to */
    void setMenu(QMenu *menu) { m_menu = menu; }

protected:
    virtual bool eventFilter(QObject *object,QEvent *event);

private slots:
    void removeDockWidget();
    /** @brief Shows a Docked Widget based on the action sender */
    void showTool(bool show);

signals:
    void needToShowDockWidget(const QString& name, bool show);
private:
    QMap<QAction*,QString > m_menuToDockNameMap;
    QList<QDockWidget*> m_dockWidgets;
    QMap<MainWindow::VIEW_SECTIONS,QMap<QString,QDockWidget*> > m_centralWidgetToDockWidgetsMap;
    bool m_isAdvancedMode; ///< If enabled dock widgets can be moved and floated.
    bool m_dockWidgetTitleBarsEnabled; ///< If enabled, dock widget titlebars are displayed when NOT in advanced mode.
    QMenu *m_menu; ///< \see setMenu()
    bool m_addedCustomSeperator; ///< Whether we have added a seperator between the actions and the custom actions

    void setDockWidgetTitleBar(QDockWidget* widget);

};

#endif // MENUACTIONHELPER_H
