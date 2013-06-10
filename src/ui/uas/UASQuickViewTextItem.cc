#include "UASQuickViewTextItem.h"
#include <QVBoxLayout>
UASQuickViewTextItem::UASQuickViewTextItem(QWidget *parent) : UASQuickViewItem(parent)
{
    QVBoxLayout *layout = new QVBoxLayout();
    this->setLayout(layout);
    layout->setSpacing(0);
    layout->setMargin(0);
    titleLabel = new QLabel(this);
    titleLabel->setAlignment(Qt::AlignHCenter);
    this->layout()->addWidget(titleLabel);
    valueLabel = new QLabel(this);
    valueLabel->setAlignment(Qt::AlignHCenter);
    valueLabel->setText("0.00");
    this->layout()->addWidget(valueLabel);
    layout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
    QFont valuefont = valueLabel->font();
    QFont titlefont = titleLabel->font();
    valuefont.setPixelSize(this->height() / 2.0);
    titlefont.setPixelSize(this->height() / 4.0);
    valueLabel->setFont(valuefont);
    titleLabel->setFont(titlefont);
}
void UASQuickViewTextItem::setValue(double value)
{
    valueLabel->setText(QString::number(value,'f',4));
}

void UASQuickViewTextItem::setTitle(QString title)
{
    titleLabel->setText(title);
}
void UASQuickViewTextItem::resizeEvent(QResizeEvent *event)
{
    QFont valuefont = valueLabel->font();
    QFont titlefont = titleLabel->font();
    valuefont.setPixelSize(this->height() / 2.0);
    titlefont.setPixelSize(this->height() / 4.0);
    valueLabel->setFont(valuefont);
    titleLabel->setFont(titlefont);
    update();
}
