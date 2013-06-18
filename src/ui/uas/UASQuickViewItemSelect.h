#ifndef UASQUICKVIEWITEMSELECT_H
#define UASQUICKVIEWITEMSELECT_H

#include <QWidget>
#include <QCheckBox>
#include "ui_UASQuickViewItemSelect.h"

class UASQuickViewItemSelect : public QWidget
{
    Q_OBJECT
    
public:
    explicit UASQuickViewItemSelect(QWidget *parent = 0);
    ~UASQuickViewItemSelect();
    void addItem(QString item,bool enabled = false);
    int currrow;
    int currcol;
protected:
    void resizeEvent(QResizeEvent *event);
private:
    QMap<QString,int> m_categoryToIndexMap;
    QMap<QCheckBox*,QString> m_checkboxToValueMap;
    QList<QCheckBox*> m_checkBoxList;
    Ui::UASQuickViewItemSelect ui;
private slots:
    void checkBoxClicked(bool checked);
signals:
    void valueEnabled(QString value);
    void valueDisabled(QString value);
};

#endif // UASQUICKVIEWITEMSELECT_H
