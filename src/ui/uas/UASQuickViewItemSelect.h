#ifndef UASQUICKVIEWITEMSELECT_H
#define UASQUICKVIEWITEMSELECT_H

#include <QWidget>
#include <QCheckBox>
#include "ui_UASQuickViewItemSelect.h"

class UASQuickViewItemSelect : public QWidget
{
    Q_OBJECT
    
public:
    explicit UASQuickViewItemSelect(bool singleonly = false,QWidget *parent = 0);
    ~UASQuickViewItemSelect();
    void addItem(QString item,bool enabled = false);
    int currrow;
    int currcol;
protected:
    void resizeEvent(QResizeEvent *event);
private:
    bool m_isSingleOnly;
    QMap<QString,int> m_categoryToIndexMap;
    QMap<QCheckBox*,QString> m_checkboxToValueMap;
    QList<QCheckBox*> m_checkBoxList;
    Ui::UASQuickViewItemSelect ui;
private slots:
    void checkBoxClicked(bool checked);
    void listItemChanged(int item);
signals:
    void valueEnabled(QString value);
    void valueDisabled(QString value);
    void valueSwapped(QString newitem,QString olditem);
};

#endif // UASQUICKVIEWITEMSELECT_H
