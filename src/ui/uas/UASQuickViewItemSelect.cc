#include "UASQuickViewItemSelect.h"
#include <QLabel>
#include <QCheckBox>
UASQuickViewItemSelect::UASQuickViewItemSelect(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    currcol = 0;
    currrow = 0;
    ui.gridLayout->setSpacing(5);
    ui.gridLayout->setMargin(0);
}
void UASQuickViewItemSelect::addItem(QString item,bool enabled)
{
    QString category = ".";
    QString name = item;
    if (item.indexOf(":") != -1 && item.indexOf(".") != -1)
    {
        //Item has a subcateogry
        category = item.mid(item.indexOf(":")+1,item.indexOf(".") - item.indexOf(":")-1);
        name = item.mid(item.indexOf(".")+1);
    }
    int col = -1;
    if (m_categoryToIndexMap.contains(category))
    {
        col = m_categoryToIndexMap[category];
    }
    else
    {
        m_categoryToIndexMap[category] = currcol++;
        col = m_categoryToIndexMap[category];
        //New column.
        QLabel *titlelabel = new QLabel(this);
        titlelabel->setText(category);
        titlelabel->show();
        ui.gridLayout->addWidget(titlelabel,0,col);
    }
    QCheckBox *label = new QCheckBox(this);
    m_checkboxToValueMap[label] = item;
    m_checkBoxList.append(label);
    if (enabled)
    {
        label->setChecked(true);
    }
    connect(label,SIGNAL(clicked(bool)),this,SLOT(checkBoxClicked(bool)));
    label->setText(name);
    label->show();
    //ui.gridLayout->addWidget(label,currrow,currcol++);
    bool breakout = false;
    int row = -1;
    while (!breakout)
    {
         if (!ui.gridLayout->itemAtPosition(++row,col) || row > 100)
         {
            breakout = true;
         }
    }
    //Row is the next invalid object, and col is the proper column.
    ui.gridLayout->addWidget(label,row,col);
}
void UASQuickViewItemSelect::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    
    /*for (int i=0;i<m_checkBoxList.size();i++)
    {
        ui.gridLayout->removeWidget(m_checkBoxList[i]);
    }
    int row = 0;
    int col = 0;
    for (int i=0;i<m_checkBoxList.size();i++)
    {
        ui.gridLayout->addWidget(m_checkBoxList[i],row,col);
        col++;
        ui.gridLayout->widget()->width() > this->width();
        //need to reduce column number.

    }*/

}

void UASQuickViewItemSelect::checkBoxClicked(bool checked)
{
    QCheckBox *check = qobject_cast<QCheckBox*>(sender());
    if (!check)
    {
        return;
    }
    QString checkval = check->text();
    if (m_checkboxToValueMap.contains(check))
    {
        checkval = m_checkboxToValueMap[check];
    }
    if (checked)
    {

        emit valueEnabled(checkval);
    }
    else
    {
        emit valueDisabled(checkval);
    }
}

UASQuickViewItemSelect::~UASQuickViewItemSelect()
{
}
