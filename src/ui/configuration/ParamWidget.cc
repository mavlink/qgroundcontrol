#include "ParamWidget.h"


ParamWidget::ParamWidget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
}

ParamWidget::~ParamWidget()
{
}

void ParamWidget::setupInt(QString title,QString description,int value,int min,int max)
{
    type = INT;
    ui.titleLabel->setText("<h3>" + title + "</h3>");
    ui.descriptionLabel->setText(description);
    ui.valueComboBox->hide();
    ui.valueSlider->show();
    ui.intSpinBox->show();
    ui.doubleSpinBox->hide();
    m_min = min;
    m_max = max;
}

void ParamWidget::setupDouble(QString title,QString description,double value,double min,double max)
{
    type = DOUBLE;
    ui.titleLabel->setText("<h3>" + title + "</h3>");
    ui.descriptionLabel->setText(description);
    ui.valueComboBox->hide();
    ui.valueSlider->show();
    ui.intSpinBox->hide();
    ui.doubleSpinBox->show();
    m_min = min;
    m_max = max;
}

void ParamWidget::setupCombo(QString title,QString description,QList<QPair<int,QString> > list)
{
    type = COMBO;
    ui.titleLabel->setText("<h3>" + title + "</h3>");
    ui.descriptionLabel->setText(description);
    ui.valueComboBox->show();
    ui.valueSlider->hide();
    ui.intSpinBox->hide();
    ui.doubleSpinBox->hide();
    m_valueList = list;
    ui.valueComboBox->clear();
    for (int i=0;i<m_valueList.size();i++)
    {
        ui.valueComboBox->addItem(m_valueList[i].second);
    }
}

void ParamWidget::setValue(double value)
{
    if (type == INT)
    {
        ui.intSpinBox->setValue(value);
        ui.valueSlider->setValue(((value + m_min) / (m_max + m_min)) * 100.0);
    }
    else if (type == DOUBLE)
    {
        ui.doubleSpinBox->setValue(value);
        ui.valueSlider->setValue(((value + m_min) / (m_max + m_min)) * 100.0);
    }
    else if (type == COMBO)
    {
        for (int i=0;i<m_valueList.size();i++)
        {
            if ((int)value == m_valueList[i].first)
            {
                ui.valueComboBox->setCurrentIndex(i);
                return;
            }
        }
    }
}
