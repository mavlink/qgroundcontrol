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
protected:
    void resizeEvent(QResizeEvent *event);
private:
    QLabel *titleLabel;
    QLabel *valueLabel;
    QSpacerItem *spacerItem;
};

#endif // UASQUICKVIEWTEXTITEM_H
