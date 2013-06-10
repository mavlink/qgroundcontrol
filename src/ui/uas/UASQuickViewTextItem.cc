#include "UASQuickViewTextItem.h"
#include <QVBoxLayout>
UASQuickViewTextItem::UASQuickViewTextItem(QWidget *parent) : UASQuickViewItem(parent)
{
    // Set a standard vertical layout.
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSizeConstraint(QLayout::SetMinimumSize);

    // Create the title label. Scale the font based on available size.
    titleLabel = new QLabel(this);
    titleLabel->setAlignment(Qt::AlignHCenter);
    titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    titleLabel->setObjectName(QString::fromUtf8("title"));
    QFont titlefont = titleLabel->font();
    titlefont.setPixelSize(this->height() / 4.0);
    titleLabel->setFont(titlefont);
    layout->addWidget(titleLabel);

    // Create the value label. Scale the font based on available size.
    valueLabel = new QLabel(this);
    valueLabel->setAlignment(Qt::AlignHCenter);
    valueLabel->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    valueLabel->setObjectName(QString::fromUtf8("value"));
    valueLabel->setText("0.00");
    QFont valuefont = valueLabel->font();
    valuefont.setPixelSize(this->height() / 2.0);
    valueLabel->setFont(valuefont);
    layout->addWidget(valueLabel);

    // And make sure the items are evenly spaced in the UASQuickView.
    layout->addSpacerItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    this->setLayout(layout);
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
