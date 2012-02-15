#ifndef VIEWPARAMWIDGET_H
#define VIEWPARAMWIDGET_H

#include <QComboBox>
#include <QDockWidget>
#include <QTabWidget>
#include <QVBoxLayout>

#include "GlobalViewParams.h"
#include "SystemViewParams.h"

class UASInterface;

class ViewParamWidget : public QDockWidget
{
    Q_OBJECT

public:
    ViewParamWidget(GlobalViewParamsPtr& globalViewParams,
                    QMap<int, SystemViewParamsPtr>& systemViewParamMap,
                    QWidget* parent = 0, QWidget* mainWindow = 0);

    void setFollowCameraId(int id);

signals:

private slots:
    void systemCreated(UASInterface* uas);

private:
    void buildLayout(QVBoxLayout* layout);
    void addTab(int systemId);

    // view parameters
    GlobalViewParamsPtr mGlobalViewParams;
    QMap<int, SystemViewParamsPtr>& mSystemViewParamMap;

    // parent widget
    QWidget* mParent;

    // child widgets
    QComboBox* mFollowCameraComboBox;
    QTabWidget* mTabWidget;
};

#endif // VIEWPARAMWIDGET_H
