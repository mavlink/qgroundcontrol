#include "UASQuickViewGaugeItem.h"
#include <QVBoxLayout>
UASQuickViewGaugeItem::UASQuickViewGaugeItem(QWidget *parent) : UASQuickViewItem(parent)
{
    
}
void UASQuickViewGaugeItem::setValue(double value)
{
    valueLabel->setText(QString::number(value,'f',4));
}

void UASQuickViewGaugeItem::setTitle(QString title)
{
    titleLabel->setText(title);
}
void UASQuickViewGaugeItem::resizeEvent(QResizeEvent *event)
{
    QFont valuefont = valueLabel->font();
    QFont titlefont = titleLabel->font();
    valuefont.setPixelSize(this->height() / 2.0);
    titlefont.setPixelSize(this->height() / 4.0);
    valueLabel->setFont(valuefont);
    titleLabel->setFont(titlefont);
    update();
}
