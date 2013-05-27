#ifndef UASQUICKVIEW_H
#define UASQUICKVIEW_H

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include "uas/UASManager.h"
#include "uas/UASInterface.h"
#include "ui_UASQuickView.h"
#include "UASQuickViewItem.h"
class UASQuickView : public QWidget
{
    Q_OBJECT
public:
    UASQuickView(QWidget *parent = 0);
private:
    UASInterface *uas;
    QList<QString> uasPropertyList;
    QMap<QString,double> uasPropertyValueMap;
    QMap<QString,UASQuickViewItem*> uasPropertyToLabelMap;
    QTimer *updateTimer;
    Ui::UASQuickView* m_ui;
signals:

public slots:
    void valueChanged(const int uasid, const QString& name, const QString& unit, const QVariant value,const quint64 msecs);
    void actionTriggered(bool checked);
    void updateTimerTick();
    void addUAS(UASInterface* uas);
    void setActiveUAS(UASInterface* uas);
    void valChanged(double val,QString type);
};

#endif // UASQUICKVIEW_H
