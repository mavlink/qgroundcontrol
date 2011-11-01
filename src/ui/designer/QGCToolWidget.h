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
    explicit QGCToolWidget(const QString& title=QString("Unnamed Tool"), QWidget *parent = 0, QSettings* settings = 0);
    ~QGCToolWidget();

    /** @brief Factory method to instantiate all tool widgets */
    static QList<QGCToolWidget*> createWidgetsFromSettings(QWidget* parent, QString settingsFile=QString());
    /** @Give the tool widget a reference to its action in the main menu */
    void setMainMenuAction(QAction* action);
    /** @brief All instances of this class */
    static QMap<QString, QGCToolWidget*>* instances();
    /** @brief Get title of widget */
    const QString getTitle();

    int isVisible(int view) { return viewVisible.value(view, false); }
    Qt::DockWidgetArea getDockWidgetArea(int view) { return dockWidgetArea.value(view, Qt::BottomDockWidgetArea); }
    void setParent(QWidget *parent);

public slots:
    void addUAS(UASInterface* uas);
    /** @brief Delete this widget */
    void deleteWidget();
    /** @brief Export this widget to a file */
    void exportWidget();
    /** @brief Import settings for this widget from a file */
    void importWidget();
    /** @brief Store all widgets of this type to QSettings */
    static void storeWidgetsToSettings(QString settingsFile=QString());
    /** @brief Load this widget from a QSettings object */
    void loadSettings(QSettings& settings);
    /** @brief Load this widget from a settings file */
    bool loadSettings(const QString& settings, bool singleinstance=false);
    /** @brief Store this widget to a QSettings object */
    void storeSettings(QSettings& settings);
    /** @brief Store this widget to a settings file */
    void storeSettings(const QString& settingsFile);
    /** @brief Store this widget to a settings file */
    void storeSettings();
    /** @brief Store the view id and dock widget area */
    void setViewVisibilityAndDockWidgetArea(int view, bool visible, Qt::DockWidgetArea area);

signals:
    void titleChanged(QString);

protected:
    QAction* addParamAction;
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
    QString widgetTitle;
    static int instanceCount;     ///< Number of instances around

    void contextMenuEvent(QContextMenuEvent* event);
    void createActions();
    QList<QGCToolWidgetItem* >* itemList();
    /** @brief Add an existing tool widget */
    void addToolWidget(QGCToolWidgetItem* widget);

    void hideEvent(QHideEvent* event);

protected slots:
    void addParam();
    void addCommand();
    void setTitle();
    void setTitle(QString title);
    void setWindowTitle(const QString& title);


private:
    Ui::QGCToolWidget *ui;
};

#endif // QGCTOOLWIDGET_H
