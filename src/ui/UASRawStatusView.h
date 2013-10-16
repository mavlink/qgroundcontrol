#ifndef UASRAWSTATUSVIEW_H
#define UASRAWSTATUSVIEW_H

#include <QWidget>
#include "MAVLinkDecoder.h"
#include "ui_UASRawStatusView.h"

class UASRawStatusView : public QWidget
{
    Q_OBJECT
    
public:
    explicit UASRawStatusView(QWidget *parent = 0);
    ~UASRawStatusView();
    void addSource(MAVLinkDecoder *decoder);
private slots:
    void updateTableTimerTick();
    void valueChanged(const int uasId, const QString& name, const QString& unit, const QVariant& value, const quint64 msec);
protected:
    void resizeEvent(QResizeEvent *event);
private:
    QMap<QString,double> valueMap;
    QMap<QString,QTableWidgetItem*> nameToUpdateWidgetMap;
    Ui::UASRawStatusView ui;
    bool m_tableDirty;
};

#endif // UASRAWSTATUSVIEW_H
