#include "UASQuickViewTextItem.h"
#include <QVBoxLayout>
#include <QDebug>
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
    layout->addSpacerItem(new QSpacerItem(10, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    this->setLayout(layout);
}
void UASQuickViewTextItem::setValue(double value)
{
    if (value < 10 && value > -10)
    {
        valueLabel->setText(QString::number(value,'f',4));
    }
    else if (value < 100 && value > -100)
    {
        valueLabel->setText(QString::number(value,'f',3));
    }
    else if (value < 1000 && value > -1000)
    {
        valueLabel->setText(QString::number(value,'f',2));
    }
    else if (value < 10000 && value > -10000)
    {
        valueLabel->setText(QString::number(value,'f',1));
    }
    else 
    {
        valueLabel->setText(QString::number(value,'f',0));
    }
}

void UASQuickViewTextItem::setTitle(QString title)
{
    if (title.indexOf(".") != -1 && title.indexOf(":") != -1)
    {
        titleLabel->setText(title.mid(title.indexOf(".") + 1));
    }
    else
    {
        titleLabel->setText(title);
    }
}
int UASQuickViewTextItem::minValuePixelSize()
{
    QFont valuefont = valueLabel->font();
    QFont titlefont = titleLabel->font();
    valuefont.setPixelSize(this->height());
    titlefont.setPixelSize(valuefont.pointSize() * 0.75);
    //spacerItem->setGeometry(QRect(0,0,20,this->height()/10.0));

    QFontMetrics metrics(valuefont);
    //valuefont.setPixelSize(this->height() / 2.0);
    bool fit = false;
    while (!fit)
    {

        QFontMetrics valfm( valuefont );
        QRect valbound = valfm.boundingRect(0,0, valueLabel->width(), valueLabel->height(), Qt::TextWordWrap | Qt::AlignLeft, "12345678.00"/*valueLabel->text()*/);
        //QFontMetrics titlefm( titlefont );
        //QRect titlebound = titlefm.boundingRect(0,0, titleLabel->width(), titleLabel->height(), Qt::TextWordWrap | Qt::AlignLeft, titleLabel->text());

        if ((valbound.width() <= valueLabel->width() && valbound.height() <= valueLabel->height())) // && (titlebound.width() <= titleLabel->width() && titlebound.height() <= titleLabel->height()))
            fit = true;
        else
        {
            if (valuefont.pointSize() - 1 <= 6)
            {
                fit = true;
                valuefont.setPixelSize(6);
            }
            else
            {
                valuefont.setPixelSize(valuefont.pointSize() - 1);
            }
            //titlefont.setPixelSize(valuefont.pointSize() / 2.0);
            //qDebug() << "Point size:" << valuefont.pointSize() << valueLabel->width() << valueLabel->height();
        }
    }
    return valuefont.pointSize();
}
void UASQuickViewTextItem::setValuePixelSize(int size)
{
    QFont valuefont = valueLabel->font();
    QFont titlefont = titleLabel->font();
    valuefont.setPixelSize(size);
    titlefont.setPixelSize(valuefont.pointSize() * 0.75);
    valueLabel->setFont(valuefont);
    titleLabel->setFont(titlefont);
    update();
}

void UASQuickViewTextItem::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    return;
#if 0
    // code ifdef'ed out to silence warnings
    QFont valuefont = valueLabel->font();
    QFont titlefont = titleLabel->font();
    valuefont.setPixelSize(this->height());
    titlefont.setPixelSize(valuefont.pointSize() / 2.0);
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
            if (valuefont.pointSize()-5 <= 0)
            {
                fit = true;
                valuefont.setPixelSize(5);
            }
            else
            {
                valuefont.setPixelSize(valuefont.pointSize() - 5);
            }
            //titlefont.setPixelSize(valuefont.pointSize() / 2.0);
            //qDebug() << "Point size:" << valuefont.pointSize() << valueLabel->width() << valueLabel->height();
        }
    }
titlefont.setPixelSize(valuefont.pointSize() / 2.0);
    valueLabel->setFont(valuefont);
    titleLabel->setFont(titlefont);
    update();
#endif
}
