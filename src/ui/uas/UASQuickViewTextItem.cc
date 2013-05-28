#include "UASQuickViewTextItem.h"
#include <QVBoxLayout>
UASQuickViewTextItem::UASQuickViewTextItem(QWidget *parent) : UASQuickViewItem(parent)
{
    QVBoxLayout *layout = new QVBoxLayout();
    this->setLayout(layout);
    titleLabel = new QLabel(this);
    titleLabel->setAlignment(Qt::AlignHCenter);
    this->layout()->addWidget(titleLabel);
    valueLabel = new QLabel(this);
    valueLabel->setAlignment(Qt::AlignHCenter);
    valueLabel->setText("<h1>0.00</h1>");
    this->layout()->addWidget(valueLabel);
    layout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
}
void UASQuickViewTextItem::setValue(double value)
{
    valueLabel->setText("<h1>" + QString::number(value,'f',4) + "</h1>");
}

void UASQuickViewTextItem::setTitle(QString title)
{
    titleLabel->setText(title);
}
