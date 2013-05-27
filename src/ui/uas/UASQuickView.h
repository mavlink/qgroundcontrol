#ifndef UASQUICKVIEW_H
#define UASQUICKVIEW_H

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include "uas/UASManager.h"
#include "uas/UASInterface.h"
#include "ui_UASQuickView.h"
#include "UASQuickViewItem.h"
#include "MAVLinkDecoder.h"
#include "UASQuickViewItemSelect.h"
class UASQuickView : public QWidget
{
    Q_OBJECT
public:
    UASQuickView(QWidget *parent = 0);
    void addSource(MAVLinkDecoder *decoder);
private:
    UASInterface *uas;
    QList<QString> uasEnabledPropertyList;
    QMap<QString,double> uasPropertyValueMap;
    QMap<QString,UASQuickViewItem*> uasPropertyToLabelMap;
    QTimer *updateTimer;
    UASQuickViewItemSelect *quickViewSelectDialog;
protected:
    Ui::Form ui;
signals:
    
public slots:
    void valueChanged(const int uasId, const QString& name, const QString& unit, const quint8 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const qint8 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const quint16 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const qint16 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const quint32 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const qint32 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const quint64 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const qint64 value, const quint64 msec);
    void valueChanged(const int uasId, const QString& name, const QString& unit, const double value, const quint64 msec);

    void valueChanged(const int uasid, const QString& name, const QString& unit, const QVariant value,const quint64 msecs);
    void actionTriggered(bool checked);
    void actionTriggered();
    void updateTimerTick();
    void addUAS(UASInterface* uas);
    void setActiveUAS(UASInterface* uas);
    void valChanged(double val,QString type);
    void selectDialogClosed();
    void valueEnabled(QString value);
    void valueDisabled(QString value);
};

#endif // UASQUICKVIEW_H
