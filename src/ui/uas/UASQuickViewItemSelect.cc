#include "UASQuickViewItemSelect.h"
#include <QLabel>
#include <QCheckBox>
UASQuickViewItemSelect::UASQuickViewItemSelect(bool singleonly,QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    currcol = 0;
    currrow = 0;
    m_isSingleOnly = singleonly;
    connect(ui.listWidget,SIGNAL(currentRowChanged(int)),this,SLOT(listItemChanged(int)));
}
void UASQuickViewItemSelect::listItemChanged(int item)
{
    if (item == -1 || ui.listWidget->count() == 0)
    {
        return;
    }
    QString value = ui.listWidget->item(item)->text();
    int index = m_categoryToIndexMap[value];
    ui.stackedWidget->setCurrentIndex(index);
}

void UASQuickViewItemSelect::addItem(QString item,bool enabled)
{
    QString category = ".";
    QString name = item;
    if (item.indexOf(".") != -1)
    {
        //Item has a subcateogry
        category = item.mid(0,item.indexOf("."));
        name = item.mid(item.indexOf(".")+1);
    }
    int col = -1;
    if (m_categoryToIndexMap.contains(category))
    {
        col = m_categoryToIndexMap[category];
    }
    else
    {

        col = ui.stackedWidget->addWidget(new QWidget(this));
        m_categoryToIndexMap[category] = col;
        ui.stackedWidget->widget(col)->setLayout(new QVBoxLayout());
        //col = m_categoryToIndexMap[category];
        //New column.
        QLabel *titlelabel = new QLabel(this);
        titlelabel->setText(category);
        titlelabel->show();
        //ui.gridLayout->addWidget(titlelabel,0,col);
        ui.stackedWidget->widget(col)->layout()->addWidget(titlelabel);

        //Ensure that GCS Status gets the top slot
        if (category == "GCS Status")
        {
            ui.listWidget->insertItem(0,"----------");
            ui.listWidget->insertItem(0,category);
        }
        else
        {
            ui.listWidget->addItem(category);
        }
        ui.stackedWidget->widget(col)->layout()->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
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
    /*    while (!breakout)
    {
         if (!ui.gridLayout->itemAtPosition(++row,col) || row > 100)
         {
            breakout = true;
         }
    }*/
    //Row is the next invalid object, and col is the proper column.
    //ui.gridLayout->addWidget(label,row,col);
    ui.stackedWidget->widget(col)->layout()->removeItem(ui.stackedWidget->widget(col)->layout()->itemAt(ui.stackedWidget->widget(col)->layout()->count()-1));
    ui.stackedWidget->widget(col)->layout()->addWidget(label);
    ui.stackedWidget->widget(col)->layout()->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
}
void UASQuickViewItemSelect::resizeEvent(QResizeEvent *event)
{
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
        if (m_isSingleOnly)
        {
            QString oldval = "";
            for (int i=0;i<m_checkBoxList.size();i++)
            {
                if (m_checkBoxList[i]->isChecked() && (checkval != m_checkboxToValueMap[m_checkBoxList[i]]))
                {
                    oldval = m_checkboxToValueMap[m_checkBoxList[i]];
                    m_checkBoxList[i]->setChecked(false);
                    break;
                }
            }
            emit valueSwapped(checkval,oldval);
            close();
        }
        else
        {
            emit valueEnabled(checkval);
        }
    }
    else
    {
        if (m_isSingleOnly)
        {
            check->setChecked(true);
        }
        else
        {
            emit valueDisabled(checkval);
        }
    }
}

UASQuickViewItemSelect::~UASQuickViewItemSelect()
{
}
