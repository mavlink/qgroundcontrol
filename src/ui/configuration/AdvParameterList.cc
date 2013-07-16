#include "AdvParameterList.h"


AdvParameterList::AdvParameterList(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    ui.tableWidget->setColumnCount(4);
    ui.tableWidget->horizontalHeader()->hide();
    ui.tableWidget->verticalHeader()->hide();
    //ui.tableWidget->setHorizontalHeader(0);
    //ui.tableWidget->setVerticalHeader(0);
    ui.tableWidget->setColumnWidth(0,200);
    ui.tableWidget->setColumnWidth(1,100);
    ui.tableWidget->setColumnWidth(2,200);
    ui.tableWidget->setColumnWidth(3,800);

}

AdvParameterList::~AdvParameterList()
{
}
void AdvParameterList::setParameterMetaData(QString name,QString humanname,QString description)
{
    paramToNameMap[name] = humanname;
    paramToDescriptionMap[name] = description;
}

void AdvParameterList::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{

    if (!paramValueMap.contains(parameterName))
    {
        ui.tableWidget->setRowCount(ui.tableWidget->rowCount()+1);
        if (paramToNameMap.contains(parameterName))
        {
            ui.tableWidget->setItem(ui.tableWidget->rowCount()-1,0,new QTableWidgetItem(paramToNameMap[parameterName]));
        }
        else
        {
            ui.tableWidget->setItem(ui.tableWidget->rowCount()-1,0,new QTableWidgetItem("Unknown"));
        }
        ui.tableWidget->setItem(ui.tableWidget->rowCount()-1,1,new QTableWidgetItem(QString::number(value.toFloat(),'f',2)));
        ui.tableWidget->setItem(ui.tableWidget->rowCount()-1,2,new QTableWidgetItem(parameterName));
        if (paramToDescriptionMap.contains(parameterName))
        {
            ui.tableWidget->setItem(ui.tableWidget->rowCount()-1,3,new QTableWidgetItem(paramToDescriptionMap[parameterName]));
        }
        else
        {
            ui.tableWidget->setItem(ui.tableWidget->rowCount()-1,3,new QTableWidgetItem("Unknown"));
        }
        paramValueMap[parameterName] = ui.tableWidget->item(ui.tableWidget->rowCount()-1,1);
    }
    paramValueMap[parameterName]->setText(QString::number(value.toFloat(),'f',2));
}
