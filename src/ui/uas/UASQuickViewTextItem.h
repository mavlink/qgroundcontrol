#ifndef UASQUICKVIEWTEXTITEM_H
#define UASQUICKVIEWTEXTITEM_H

#include "UASQuickViewItem.h"
#include <QLabel>
#include <QSpacerItem>

class UASQuickViewTextItem : public UASQuickViewItem
{
    Q_OBJECT

public:
    explicit UASQuickViewTextItem(QWidget *parent=0);
    virtual ~UASQuickViewTextItem();
    void setValue(double value) override;
    void setTitle(QString title) override;
    int minValuePixelSize() override;
    void setValuePixelSize(int size) override;


protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel *titleLabel;
    QLabel *valueLabel;
};

#endif // UASQUICKVIEWTEXTITEM_H
