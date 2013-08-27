#ifndef UASQUICKVIEWITEM_H
#define UASQUICKVIEWITEM_H

#include <QWidget>
class UASQuickViewItem : public QWidget
{
    Q_OBJECT
public:
    explicit UASQuickViewItem(QWidget *parent = 0);
    virtual void setValue(double value)=0;
    virtual void setTitle(QString title)=0;
    virtual int minValuePixelSize()=0;
    virtual void setValuePixelSize(int size)=0;
};

#endif // UASQUICKVIEWITEM_H
