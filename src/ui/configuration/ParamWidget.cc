#include "ParamWidget.h"

ParamWidget::ParamWidget(QString param,QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    m_param = param;

    connect(ui.doubleSpinBox,SIGNAL(editingFinished()),this,SLOT(doubleSpinEditFinished()));
    connect(ui.intSpinBox,SIGNAL(editingFinished()),this,SLOT(intSpinEditFinished()));
    connect(ui.valueComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(comboIndexChanged(int)));
    connect(ui.valueSlider,SIGNAL(sliderReleased()),this,SLOT(valueSliderReleased()));
}
void ParamWidget::doubleSpinEditFinished()
{
    ui.valueSlider->setValue(((ui.doubleSpinBox->value() - m_min) / (m_max - m_min)) * 100.0);
    emit doubleValueChanged(m_param,ui.doubleSpinBox->value());
}

void ParamWidget::intSpinEditFinished()
{
    ui.valueSlider->setValue(((ui.intSpinBox->value() - m_min) / (m_max - m_min)) * 100.0);
    emit intValueChanged(m_param,ui.intSpinBox->value());
}

void ParamWidget::comboIndexChanged(int index)
{
    emit intValueChanged(m_param,m_valueList[index].first);
}

void ParamWidget::valueSliderReleased()
{
    //Set the spin box, and emit a signal.
    if (type == INT)
    {
        ui.intSpinBox->setValue(((ui.valueSlider->value() / 100.0) * (m_max - m_min)) + m_min);
        emit intValueChanged(m_param,ui.intSpinBox->value());
    }
    else if (type == DOUBLE)
    {
        ui.doubleSpinBox->setValue(((ui.valueSlider->value() / 100.0) * (m_max - m_min)) + m_min);
        emit doubleValueChanged(m_param,ui.doubleSpinBox->value());
    }
}

ParamWidget::~ParamWidget()
{
}

void ParamWidget::setupInt(QString title,QString description,int value,int min,int max)
{
    Q_UNUSED(value);
    
    type = INT;
    ui.titleLabel->setText("<h3>" + title + "</h3>");
    ui.descriptionLabel->setText(description);
    ui.valueComboBox->hide();
    ui.valueSlider->show();
    ui.intSpinBox->show();
    ui.doubleSpinBox->hide();
    if (min == 0 && max == 0)
    {
        m_min = 0;
        m_max = 65535;
    }
    else
    {
        m_min = min;
        m_max = max;
    }
    ui.intSpinBox->setMinimum(m_min);
    ui.intSpinBox->setMaximum(m_max);
}

void ParamWidget::setupDouble(QString title,QString description,double value,double min,double max)
{
    Q_UNUSED(value);
    
    type = DOUBLE;
    ui.titleLabel->setText("<h3>" + title + "</h3>");
    ui.descriptionLabel->setText(description);
    ui.valueComboBox->hide();
    ui.valueSlider->show();
    ui.intSpinBox->hide();
    ui.doubleSpinBox->show();
    if (min == 0 && max == 0)
    {
        m_min = 0;
        m_max = 65535;
    }
    else
    {
        m_min = min;
        m_max = max;
    }
    ui.doubleSpinBox->setMinimum(m_min);
    ui.doubleSpinBox->setMaximum(m_max);
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
    disconnect(ui.valueComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(comboIndexChanged(int)));
    for (int i=0;i<m_valueList.size();i++)
    {
        ui.valueComboBox->addItem(m_valueList[i].second);
    }
    connect(ui.valueComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(comboIndexChanged(int)));
}

void ParamWidget::setValue(double value)
{
    if (type == INT)
    {
        ui.intSpinBox->setValue(value);
        ui.valueSlider->setValue(((value - m_min) / (m_max - m_min)) * 100.0);
    }
    else if (type == DOUBLE)
    {
        ui.doubleSpinBox->setValue(value);
        ui.valueSlider->setValue(((value - m_min) / (m_max - m_min)) * 100.0);
    }
    else if (type == COMBO)
    {
        for (int i=0;i<m_valueList.size();i++)
        {
            if ((int)value == m_valueList[i].first)
            {
                disconnect(ui.valueComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(comboIndexChanged(int)));
                ui.valueComboBox->setCurrentIndex(i);
                connect(ui.valueComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(comboIndexChanged(int)));
                return;
            }
        }
    }
}
