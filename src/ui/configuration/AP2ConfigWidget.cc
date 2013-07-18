#include <QMessageBox>
#include "AP2ConfigWidget.h"

AP2ConfigWidget::AP2ConfigWidget(QWidget *parent) : QWidget(parent)
{
    m_uas = 0;
    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(activeUASSet(UASInterface*)));
    activeUASSet(UASManager::instance()->getActiveUAS());
}
void AP2ConfigWidget::activeUASSet(UASInterface *uas)
{
    if (m_uas)
    {
        disconnect(m_uas,SIGNAL(parameterChanged(int,int,QString,QVariant)),this,SLOT(parameterChanged(int,int,QString,QVariant)));
        m_uas = 0;
    }
    if (!uas) return;
    m_uas = uas;
    connect(m_uas,SIGNAL(parameterChanged(int,int,QString,QVariant)),this,SLOT(parameterChanged(int,int,QString,QVariant)));
}

void AP2ConfigWidget::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{

}
void AP2ConfigWidget::showNullMAVErrorMessageBox()
{
    QMessageBox::information(0,tr("Error"),tr("Please connect to a MAV before attempting to set configuration"));
}
