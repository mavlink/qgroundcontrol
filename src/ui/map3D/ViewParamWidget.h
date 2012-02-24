#ifndef VIEWPARAMWIDGET_H
#define VIEWPARAMWIDGET_H

#include <QComboBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QSignalMapper>
#include <QSpinBox>
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
    void overlayCreated(int systemId, const QString& name);
    void systemCreated(UASInterface* uas);
    void setpointsCheckBoxToggled(int state);
    void showImageryParamDialog(void);

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
    QSpinBox* mSetpointHistoryLengthSpinBox;
    QMap<int, QFormLayout*> mOverlayLayout;
    QTabWidget* mTabWidget;

    QSignalMapper* mOverlaySignalMapper;
};

#endif // VIEWPARAMWIDGET_H
