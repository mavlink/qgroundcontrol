#ifndef UASQUICKVIEWTEXTITEM_H
#define UASQUICKVIEWTEXTITEM_H

#include "UASQuickViewItem.h"
#include <QLabel>
#include <QSpacerItem>
class UASQuickViewTextItem : public UASQuickViewItem
{
public:
    UASQuickViewTextItem(QWidget *parent=0);
    void setValue(double value);
    void setTitle(QString title);
    int minValuePixelSize();
    void setValuePixelSize(int size);
protected:
    void resizeEvent(QResizeEvent *event);
private:
    QLabel *titleLabel;
    QLabel *valueLabel;
};

#endif // UASQUICKVIEWTEXTITEM_H
