#ifndef ARDUCOPTERPIDCONFIG_H
#define ARDUCOPTERPIDCONFIG_H

#include <QWidget>
#include "ui_ArduCopterPidConfig.h"

#include "AP2ConfigWidget.h"

class ArduCopterPidConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit ArduCopterPidConfig(QWidget *parent = 0);
    ~ArduCopterPidConfig();
private slots:
    void writeButtonClicked();
    void refreshButtonClicked();
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
private:
    QList<QPair<int,QString> > m_ch6ValueToTextList;
    QList<QPair<int,QString> > m_ch78ValueToTextList;
    QMap<QString,QDoubleSpinBox*> m_nameToBoxMap;
    Ui::ArduCopterPidConfig ui;
};

#endif // ARDUCOPTERPIDCONFIG_H
