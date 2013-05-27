#include "UASQuickViewItem.h"
#include <QVBoxLayout>
#include <QStyleOption>
#include <QPainter>
#include <QSizePolicy>

UASQuickViewItem::UASQuickViewItem(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSizeConstraint(QLayout::SetMinimumSize);

    titleLabel = new QLabel(this);
    titleLabel->setAlignment(Qt::AlignHCenter);
    titleLabel->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    titleLabel->setObjectName(QString::fromUtf8("title"));
    layout->addWidget(titleLabel);

    valueLabel = new QLabel(this);
    valueLabel->setAlignment(Qt::AlignHCenter);
    valueLabel->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Minimum);
    valueLabel->setObjectName(QString::fromUtf8("value"));
    valueLabel->setText("0.00");
    layout->addWidget(valueLabel);

    layout->addSpacerItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    this->setLayout(layout);
}

void UASQuickViewItem::setValue(double value)
{
    valueLabel->setText(QString::number(value,'f',4));
}

void UASQuickViewItem::setTitle(QString title)
{
    titleLabel->setText(title);
}

/**
 * Implement paintEvent() so that stylesheets work for our custom widget.
 */
void UASQuickViewItem::paintEvent(QPaintEvent *)
 {
     QStyleOption opt;
     opt.init(this);
     QPainter p(this);
     style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
 }
