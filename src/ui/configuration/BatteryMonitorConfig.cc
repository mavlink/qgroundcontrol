#include "BatteryMonitorConfig.h"
#include <QMessageBox>

BatteryMonitorConfig::BatteryMonitorConfig(QWidget *parent) : AP2ConfigWidget(parent)
{
    ui.setupUi(this);
    ui.monitorComboBox->addItem(tr("0: Disabled"));
    ui.monitorComboBox->addItem(tr("3: Battery Volts"));
    ui.monitorComboBox->addItem(tr("4: Voltage and Current"));

    ui.sensorComboBox->addItem(tr("0: Other"));
    ui.sensorComboBox->addItem("1: AttoPilot 45A");
    ui.sensorComboBox->addItem("2: AttoPilot 90A");
    ui.sensorComboBox->addItem("3: AttoPilot 180A");
    ui.sensorComboBox->addItem("4: 3DR Power Module");

    ui.apmVerComboBox->addItem("0: APM1");
    ui.apmVerComboBox->addItem("1: APM2 - 2.5 non 3DR");
    ui.apmVerComboBox->addItem("2: APM2.5 - 3DR Power Module");
    ui.apmVerComboBox->addItem("3: PX4");

    ui.alertOnLowCheckBox->setVisible(false); //Unimpelemented, but TODO.


    connect(ui.monitorComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(monitorCurrentIndexChanged(int)));
    connect(ui.sensorComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(sensorCurrentIndexChanged(int)));
    connect(ui.apmVerComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(apmVerCurrentIndexChanged(int)));

    connect(ui.calcDividerLineEdit,SIGNAL(editingFinished()),this,SLOT(calcDividerSet()));
    connect(ui.ampsPerVoltsLineEdit,SIGNAL(editingFinished()),this,SLOT(ampsPerVoltSet()));
    connect(ui.battCapacityLineEdit,SIGNAL(editingFinished()),this,SLOT(batteryCapacitySet()));


    initConnections();
}
void BatteryMonitorConfig::activeUASSet(UASInterface *uas)
{
    if (m_uas)
    {
        disconnect(m_uas,SIGNAL(batteryChanged(UASInterface*,double,double,double,int)),this,SLOT(batteryChanged(UASInterface*,double,double,double,int)));
    }
    AP2ConfigWidget::activeUASSet(uas);
    if (!uas)
    {
        return;
    }
    connect(uas,SIGNAL(batteryChanged(UASInterface*,double,double,double,int)),this,SLOT(batteryChanged(UASInterface*,double,double,double,int)));

}
void BatteryMonitorConfig::alertOnLowClicked(bool checked)
{
    Q_UNUSED(checked);
}

void BatteryMonitorConfig::calcDividerSet()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    bool ok = false;
    float newval = ui.calcDividerLineEdit->text().toFloat(&ok);
    if (!ok)
    {
        //Error parsing;
        QMessageBox::information(0,"Error","Invalid number entered for voltage divider. Please try again");
        return;
    }
    m_uas->getParamManager()->setParameter(1,"VOLT_DIVIDER",newval);
}
void BatteryMonitorConfig::ampsPerVoltSet()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    bool ok = false;
    float newval = ui.ampsPerVoltsLineEdit->text().toFloat(&ok);
    if (!ok)
    {
        //Error parsing;
        QMessageBox::information(0,"Error","Invalid number entered for amps per volts. Please try again");
        return;
    }
    m_uas->getParamManager()->setParameter(1,"AMPS_PER_VOLT",newval);
}
void BatteryMonitorConfig::batteryCapacitySet()
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    bool ok = false;
    float newval = ui.battCapacityLineEdit->text().toFloat(&ok);
    if (!ok)
    {
        //Error parsing;
        QMessageBox::information(0,"Error","Invalid number entered for amps per volts. Please try again");
        return;
    }
    m_uas->getParamManager()->setParameter(1,"BATT_CAPACITY",newval);
}

void BatteryMonitorConfig::monitorCurrentIndexChanged(int index)
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    if (index == 0) //Battery Monitor Disabled
    {
        m_uas->getParamManager()->setParameter(1,"BATT_VOLT_PIN",-1);
        m_uas->getParamManager()->setParameter(1,"BATT_CURR_PIN",-1);
        m_uas->getParamManager()->setParameter(1,"BATT_MONITOR",0);
        ui.sensorComboBox->setEnabled(false);
        ui.apmVerComboBox->setEnabled(false);
        ui.measuredVoltsLineEdit->setEnabled(false);
        ui.measuredVoltsLineEdit->setEnabled(false);
        ui.calcDividerLineEdit->setEnabled(false);
        ui.calcVoltsLineEdit->setEnabled(false);
        ui.ampsPerVoltsLineEdit->setEnabled(false);
    }
    else if (index == 1) //Monitor voltage only
    {
        m_uas->getParamManager()->setParameter(1,"BATT_MONITOR",3);
        ui.sensorComboBox->setEnabled(false);
        ui.apmVerComboBox->setEnabled(true);
        ui.measuredVoltsLineEdit->setEnabled(true);
        ui.calcDividerLineEdit->setEnabled(true);
        ui.calcVoltsLineEdit->setEnabled(false);
        ui.ampsPerVoltsLineEdit->setEnabled(false);
    }
    else if (index == 2) //Monitor voltage and current
    {
        m_uas->getParamManager()->setParameter(1,"BATT_MONITOR",4);
        ui.sensorComboBox->setEnabled(true);
        ui.apmVerComboBox->setEnabled(true);
        ui.measuredVoltsLineEdit->setEnabled(true);
        ui.calcDividerLineEdit->setEnabled(true);
        ui.calcVoltsLineEdit->setEnabled(false);
        ui.ampsPerVoltsLineEdit->setEnabled(true);
    }


}
void BatteryMonitorConfig::sensorCurrentIndexChanged(int index)
{
    float maxvolt = 0.0f;
    float maxamps = 0.0f;
    float mvpervolt = 0.0f;
    float mvperamp = 0.0f;
    float topvolt = 0.0f;
    float topamps = 0.0f;

    if (index == 1)
    {
        //atto 45 see https://www.sparkfun.com/products/10643
        maxvolt = 13.6f;
        maxamps = 44.7f;
    }
    else if (index == 2)
    {
        //atto 90 see https://www.sparkfun.com/products/9028
        maxvolt = 51.8f;
        maxamps = 89.4f;
    }
    else if (index == 3)
    {
        //atto 180 see https://www.sparkfun.com/products/10644
        maxvolt = 51.8f;
        maxamps = 178.8f;
    }
    else if (index == 4)
    {
        //3dr
        maxvolt = 50.0f;
        maxamps = 90.0f;
    }
    mvpervolt = calculatemVPerVolt(3.3f, maxvolt);
    mvperamp = calculatemVPerAmp(3.3f, maxamps);
    if (index == 0)
    {
        //Other
        ui.ampsPerVoltsLineEdit->setEnabled(true);
        ui.calcDividerLineEdit->setEnabled(true);
        ui.measuredVoltsLineEdit->setEnabled(true);
    }
    else
    {
        topvolt = (maxvolt * mvpervolt) / 1000.0;
        topamps = (maxamps * mvperamp) / 1000.0;

        ui.calcDividerLineEdit->setText(QString::number(maxvolt/topvolt));
        ui.ampsPerVoltsLineEdit->setText(QString::number(maxamps / topamps));
        ui.ampsPerVoltsLineEdit->setEnabled(false);
        ui.calcDividerLineEdit->setEnabled(false);
        ui.measuredVoltsLineEdit->setEnabled(false);
    }
}
float BatteryMonitorConfig::calculatemVPerAmp(float maxvoltsout,float maxamps)
{
    return (1000.0 * (maxvoltsout/maxamps));
}

float BatteryMonitorConfig::calculatemVPerVolt(float maxvoltsout,float maxvolts)
{
    return (1000.0 * (maxvoltsout/maxvolts));
}

void BatteryMonitorConfig::apmVerCurrentIndexChanged(int index)
{
    if (!m_uas)
    {
        showNullMAVErrorMessageBox();
        return;
    }
    if (index == 0) //APM1
    {
        m_uas->getParamManager()->setParameter(1,"BATT_VOLT_PIN",0);
        m_uas->getParamManager()->setParameter(1,"BATT_CURR_PIN",1);
    }
    else if (index == 1) //APM2
    {
        m_uas->getParamManager()->setParameter(1,"BATT_VOLT_PIN",1);
        m_uas->getParamManager()->setParameter(1,"BATT_CURR_PIN",2);
    }
    else if (index == 2) //APM2.5
    {
        m_uas->getParamManager()->setParameter(1,"BATT_VOLT_PIN",13);
        m_uas->getParamManager()->setParameter(1,"BATT_CURR_PIN",12);
    }
    else if (index == 3) //PX4
    {
        m_uas->getParamManager()->setParameter(1,"BATT_VOLT_PIN",100);
        m_uas->getParamManager()->setParameter(1,"BATT_CURR_PIN",101);
        m_uas->getParamManager()->setParameter(1,"VOLT_DIVIDER",1);
        ui.calcDividerLineEdit->setText("1");
    }
}

BatteryMonitorConfig::~BatteryMonitorConfig()
{
}
void BatteryMonitorConfig::batteryChanged(UASInterface* uas, double voltage, double current, double percent, int seconds)
{
    Q_UNUSED(uas);
    Q_UNUSED(current);
    Q_UNUSED(percent);
    Q_UNUSED(seconds);
    
    ui.calcVoltsLineEdit->setText(QString::number(voltage,'f',2));
    if (ui.measuredVoltsLineEdit->text() == "")
    {
        ui.measuredVoltsLineEdit->setText(ui.calcVoltsLineEdit->text());
    }
}

void BatteryMonitorConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    Q_UNUSED(uas);
    Q_UNUSED(component);
    
    if (parameterName == "VOLT_DIVIDER")
    {
        ui.calcDividerLineEdit->setText(QString::number(value.toFloat(),'f',4));
    }
    else if (parameterName == "AMP_PER_VOLT")
    {
        ui.ampsPerVoltsLineEdit->setText(QString::number(value.toFloat(),'g',2));

    }
    else if (parameterName == "BATT_MONITOR")
    {
        if (value.toInt() == 0) //0: Disable
        {
            ui.monitorComboBox->setCurrentIndex(0);
        }
        else if (value.toInt() == 3) //3: Battery volts
        {
            ui.monitorComboBox->setCurrentIndex(1);
        }
        else if (value.toInt() == 4) //4: Voltage and Current
        {
            ui.monitorComboBox->setCurrentIndex(2);
        }

    }
    else if (parameterName == "BATT_CAPACITY")
    {
        ui.battCapacityLineEdit->setText(QString::number(value.toFloat()));
    }
    else if (parameterName == "BATT_VOLT_PIN")
    {
        int ivalue = value.toInt();
        if (ivalue == 0) //APM1
        {
            ui.apmVerComboBox->setCurrentIndex(0);
        }
        else if (ivalue == 1) //APM2
        {
            ui.apmVerComboBox->setCurrentIndex(1);
        }
        else if (ivalue == 13) //APM2.5
        {
            ui.apmVerComboBox->setCurrentIndex(2);
        }
        else if (ivalue == 100) //PX4
        {
            ui.apmVerComboBox->setCurrentIndex(3);
        }
    }
    else if (parameterName == "BATT_CURR_PIN")
    {
        //Unused at the moment, everything is off BATT_VOLT_PIN
    }
}
