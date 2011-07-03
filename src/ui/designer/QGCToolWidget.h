#ifndef QGCTOOLWIDGET_H
#define QGCTOOLWIDGET_H

#include <QWidget>
#include <QAction>
#include <QMap>
#include <QVBoxLayout>
#include "QGCToolWidgetItem.h"

#include "UAS.h"

namespace Ui
{
class QGCToolWidget;
}

class QGCToolWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QGCToolWidget(const QString& title, QWidget *parent = 0);
    ~QGCToolWidget();

    /** @brief Factory method to instantiate all tool widgets */
    static QList<QGCToolWidget*> createWidgetsFromSettings(QWidget* parent);
    /** @Give the tool widget a reference to its action in the main menu */
    void setMainMenuAction(QAction* action);
    /** @brief All instances of this class */
    static QMap<QString, QGCToolWidget*>* instances();

    int isVisible(int view) { return viewVisible.value(view, false); }
    Qt::DockWidgetArea getDockWidgetArea(int view) { return dockWidgetArea.value(view, Qt::BottomDockWidgetArea); }

public slots:
    void addUAS(UASInterface* uas);
    /** @brief Delete this widget */
    void deleteWidget();
    /** @brief Export this widget to a file */
    void exportWidget();
    /** @brief Import settings for this widget from a file */
    void importWidget(const QString& fileName);
    /** @brief Store all widgets of this type to QSettings */
    static void storeWidgetsToSettings();
    /** @brief Store the view id and dock widget area */
    void setViewVisibilityAndDockWidgetArea(int view, bool visible, Qt::DockWidgetArea area);

signals:
    void titleChanged(QString);

protected:
    QAction* addParamAction;
    QAction* addButtonAction;
    QAction* addCommandAction;
    QAction* setTitleAction;
    QAction* deleteAction;
    QAction* exportAction;
    QAction* importAction;
    QVBoxLayout* toolLayout;
    UAS* mav;
    QAction* mainMenuAction;             ///< Main menu action
    QMap<int, Qt::DockWidgetArea> dockWidgetArea;   ///< Dock widget area desired by this widget
    QMap<int, bool> viewVisible;  ///< Visibility in one view

    void contextMenuEvent(QContextMenuEvent* event);
    void createActions();
    QList<QGCToolWidgetItem* >* itemList();
    const QString getTitle();
    /** @brief Add an existing tool widget */
    void addToolWidget(QGCToolWidgetItem* widget);

    void hideEvent(QHideEvent* event);

protected slots:
    void addParam();
    /** @deprecated */
    void addAction();
    void addCommand();
    void setTitle();


private:
    Ui::QGCToolWidget *ui;
};

#endif // QGCTOOLWIDGET_H
