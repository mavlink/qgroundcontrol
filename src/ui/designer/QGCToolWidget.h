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
    explicit QGCToolWidget(const QString& objectName, const QString& title, QWidget *parent = 0, QSettings* settings = 0);
    ~QGCToolWidget();

    /** @brief Factory method to instantiate all tool widgets */
    static QList<QGCToolWidget*> createWidgetsFromSettings(QWidget* parent, QString settingsFile=QString());
    /** @Give the tool widget a reference to its action in the main menu */
    void setMainMenuAction(QAction* action);
    /** @brief All instances of this class */
    static QMap<QString, QGCToolWidget*>* instances();
    /** @brief Get title of widget */
    QString getTitle() const;

    int isVisible(int view) const { return viewVisible.value(view, false); }
    Qt::DockWidgetArea getDockWidgetArea(int view) { return dockWidgetArea.value(view, Qt::BottomDockWidgetArea); }
    void setParent(QWidget *parent);

    /** @brief Store all widgets of this type to QSettings */
    static void storeWidgetsToSettings(QSettings &settingsFile);

public slots:
    void addUAS(UASInterface* uas);
    /** @brief Delete this widget */
    void deleteWidget();
    /** @brief Export this widget to a file */
    void exportWidget();
    /** @brief Import settings for this widget from a file */
    void importWidget();
    /** @brief Store all widgets of this type to QSettings */
    void storeWidgetsToSettings() { QSettings settings; QGCToolWidget::storeWidgetsToSettings(settings); }
    void showLabel(QString name,int num);

public:
    void loadSettings(QVariantMap& settings);
    /** @brief Load this widget from a QSettings object */
    void loadSettings(QSettings& settings);
    /** @brief Load this widget from a settings file */
    bool loadSettings(const QString& settings, bool singleinstance=false);
    /** @brief Store the view id and dock widget area */
    void setViewVisibilityAndDockWidgetArea(int view, bool visible, Qt::DockWidgetArea area);
    void setSettings(QVariantMap& settings);
    QList<QString> getParamList();
    void setParameterValue(int uas, int component, QString parameterName, const QVariant value);
    bool fromMetaData() const { return isFromMetaData; }

signals:
    void titleChanged(const QString &title);

protected:
    bool isFromMetaData;
    QMap<QString,QGCToolWidgetItem*> paramToItemMap;
    QList<QGCToolWidgetItem*> toolItemList;
    QList<QString> paramList;
    QVariantMap settingsMap;
    QAction* addParamAction;
    QAction* addCommandAction;
    QAction* addPlotAction;
    QAction* addLabelAction;
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

    void contextMenuEvent(QContextMenuEvent* event);
    void createActions();
    /** @brief Add an existing tool widget */
    void addToolWidget(QGCToolWidgetItem* widget);
    /** @brief Add an existing tool widget and set it to edit mode */
    void addToolWidgetAndEdit(QGCToolWidgetItem* widget);

    void hideEvent(QHideEvent* event);
public slots:
    void setTitle(const QString &title);
    void addParam(int uas,int component,QString paramname,QVariant value);
protected slots:
    void addParam();
    void addCommand();
    void addPlot();
    void addLabel();
    void setTitle();
    void widgetRemoved();

private:
    /** Do not use this from outside the class to set the object name,
     * because we cannot track changes to the object name, and the
     * QObject::setObjectName() function is not virtual. Instead only
     * pass in the object name to the constructor, or use the , then
     * never change it again. */
    void setObjectName(const QString &name) { QWidget::setObjectName(name); }
    /** Helper for storeWidgetsToSettings() */
    void storeSettings(QSettings& settings);

    Ui::QGCToolWidget *ui;
};

#endif // QGCTOOLWIDGET_H
