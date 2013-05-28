#include "UASQuickViewItemSelect.h"
#include <QLabel>
#include <QCheckBox>
UASQuickViewItemSelect::UASQuickViewItemSelect(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    currcol = 0;
    currrow = 0;
}
void UASQuickViewItemSelect::addItem(QString item,bool enabled)
{
    QCheckBox *label = new QCheckBox(this);
    if (enabled)
    {
        label->setChecked(true);
    }
    connect(label,SIGNAL(clicked(bool)),this,SLOT(checkBoxClicked(bool)));
    label->setText(item);
    label->show();
    ui.gridLayout->addWidget(label,currrow,currcol++);
    if (currcol > 10)
    {
        currcol = 0;
        currrow++;
    }
}
void UASQuickViewItemSelect::checkBoxClicked(bool checked)
{
    QCheckBox *check = qobject_cast<QCheckBox*>(sender());
    if (!check)
    {
        return;
    }
    if (checked)
    {
        emit valueEnabled(check->text());
    }
    else
    {
        emit valueDisabled(check->text());
    }
}

UASQuickViewItemSelect::~UASQuickViewItemSelect()
{
}
