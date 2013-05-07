#ifndef UASQUICKVIEWITEM_H
#define UASQUICKVIEWITEM_H

#include <QWidget>
#include <QLabel>
class UASQuickViewItem : public QWidget
{
    Q_OBJECT
public:
    explicit UASQuickViewItem(QWidget *parent = 0);
    void setValue(double value);
    void setTitle(QString title);
private:
    QLabel *titleLabel;
    QLabel *valueLabel;
signals:
    
public slots:
    
};

#endif // UASQUICKVIEWITEM_H
