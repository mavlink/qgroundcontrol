#ifndef PARAMWIDGET_H
#define PARAMWIDGET_H

#include <QWidget>
#include "ui_ParamWidget.h"

class ParamWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit ParamWidget(QWidget *parent = 0);
    ~ParamWidget();
    void setupInt(QString title,QString description,int value,int min,int max);
    void setupDouble(QString title,QString description,double value,double min,double max);
    void setupCombo(QString title,QString description,QList<QPair<int,QString> > list);
    void setValue(double value);
private:
    enum VIEWTYPE
    {
        INT,
        DOUBLE,
        COMBO
    };
    double m_min;
    double m_max;
    double m_dvalue;
    int m_ivalue;
    VIEWTYPE type;
    QList<QPair<int,QString> > m_valueList;
    Ui::ParamWidget ui;
};

#endif // PARAMWIDGET_H
