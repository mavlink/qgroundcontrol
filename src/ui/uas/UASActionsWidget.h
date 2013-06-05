#ifndef UASACTIONSWIDGET_H
#define UASACTIONSWIDGET_H

#include <QWidget>
#include "ui_UASActionsWidget.h"
#include <UASManager.h>
#include <UASInterface.h>
class UASActionsWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit UASActionsWidget(QWidget *parent = 0);
    ~UASActionsWidget();
    
private:
    Ui::UASActionsWidget ui;
    UASInterface *m_uas;
private slots:
    void armButtonClicked();
    void armingChanged(bool state);
    void currentWaypointChanged(quint16 wpid);
    void updateWaypointList();
    void activeUASSet(UASInterface *uas);
    void goToWaypointClicked();
    void changeAltitudeClicked();
    void changeSpeedClicked();
};

#endif // UASACTIONSWIDGET_H
