#include "UASQuickViewTextItem.h"
#include <QVBoxLayout>
#include <QDebug>
UASQuickViewTextItem::UASQuickViewTextItem(QWidget *parent) : UASQuickViewItem(parent)
{
    QVBoxLayout *layout = new QVBoxLayout();
    this->setLayout(layout);
    layout->setSpacing(0);
    layout->setMargin(0);
    titleLabel = new QLabel(this);
    titleLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);
    titleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    this->layout()->addWidget(titleLabel);
    valueLabel = new QLabel(this);
    valueLabel->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);
    valueLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    valueLabel->setText("0.00");
    this->layout()->addWidget(valueLabel);
    //spacerItem = new QSpacerItem(20,40,QSizePolicy::Minimum,QSizePolicy::Ignored);
    //layout->addSpacerItem(spacerItem);
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
    valuefont.setPixelSize(this->height());
    titlefont.setPixelSize(valuefont.pixelSize() / 2.0);
    //spacerItem->setGeometry(QRect(0,0,20,this->height()/10.0));

    QFontMetrics metrics(valuefont);
    //valuefont.setPixelSize(this->height() / 2.0);
    bool fit = false;
    while (!fit)
    {

        QFontMetrics valfm( valuefont );
        QRect valbound = valfm.boundingRect(0,0, valueLabel->width(), valueLabel->height(), Qt::TextWordWrap | Qt::AlignLeft, valueLabel->text());
        //QFontMetrics titlefm( titlefont );
        //QRect titlebound = titlefm.boundingRect(0,0, titleLabel->width(), titleLabel->height(), Qt::TextWordWrap | Qt::AlignLeft, titleLabel->text());

        if ((valbound.width() <= valueLabel->width() && valbound.height() <= valueLabel->height()))// && (titlebound.width() <= titleLabel->width() && titlebound.height() <= titleLabel->height()))
            fit = true;
        else
        {
            if (valuefont.pixelSize()-5 <= 0)
            {
                fit = true;
                valuefont.setPixelSize(5);
            }
            else
            {
                valuefont.setPixelSize(valuefont.pixelSize() - 5);
            }
            //titlefont.setPixelSize(valuefont.pixelSize() / 2.0);
            //qDebug() << "Point size:" << valuefont.pixelSize() << valueLabel->width() << valueLabel->height();
        }
    }
titlefont.setPixelSize(valuefont.pixelSize() / 2.0);
    valueLabel->setFont(valuefont);
    titleLabel->setFont(titlefont);
    update();
}
