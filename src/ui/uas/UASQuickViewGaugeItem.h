#ifndef UASQuickViewGaugeItem_H
#define UASQuickViewGaugeItem_H

#include "UASQuickViewItem.h"
#include <QLabel>
class UASQuickViewGaugeItem : public UASQuickViewItem
{
public:
    UASQuickViewGaugeItem(QWidget *parent=0);
    void setValue(double value);
    void setTitle(QString title);
protected:
    void resizeEvent(QResizeEvent *event);
private:
    QLabel *titleLabel;
    QLabel *valueLabel;
};

#endif // UASQuickViewGaugeItem_H
