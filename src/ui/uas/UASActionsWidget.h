#ifndef UASACTIONSWIDGET_H
#define UASACTIONSWIDGET_H

#include <QWidget>
#include "ui_UASActionsWidget.h"

class UASActionsWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit UASActionsWidget(QWidget *parent = 0);
    ~UASActionsWidget();
    
private:
    Ui::UASActionsWidget ui;
};

#endif // UASACTIONSWIDGET_H
