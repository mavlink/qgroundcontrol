#ifndef ADVPARAMETERLIST_H
#define ADVPARAMETERLIST_H

#include <QWidget>
#include "ui_AdvParameterList.h"
#include "AP2ConfigWidget.h"

class AdvParameterList : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit AdvParameterList(QWidget *parent = 0);
    void setParameterMetaData(QString name,QString humanname,QString description);
    ~AdvParameterList();
private slots:
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
private:
    QMap<QString,QTableWidgetItem*> m_paramValueMap;
    QMap<QString,QString> m_paramToNameMap;
    QMap<QString,QString> m_paramToDescriptionMap;
    Ui::AdvParameterList ui;
};

#endif // ADVPARAMETERLIST_H
