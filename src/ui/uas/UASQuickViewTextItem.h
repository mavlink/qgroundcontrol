#ifndef UASQUICKVIEWTEXTITEM_H
#define UASQUICKVIEWTEXTITEM_H

#include "UASQuickViewItem.h"
#include <QLabel>
class UASQuickViewTextItem : public UASQuickViewItem
{
public:
    UASQuickViewTextItem(QWidget *parent=0);
    void setValue(double value);
    void setTitle(QString title);
protected:
    void resizeEvent(QResizeEvent *event);
private:
    QLabel *titleLabel;
    QLabel *valueLabel;
};

#endif // UASQUICKVIEWTEXTITEM_H
