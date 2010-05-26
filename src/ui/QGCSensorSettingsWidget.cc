#include "QGCSensorSettingsWidget.h"
#include "ui_QGCSensorSettingsWidget.h"

QGCSensorSettingsWidget::QGCSensorSettingsWidget(UASInterface* uas, QWidget *parent) :
    QWidget(parent),
    mav(uas),
    ui(new Ui::QGCSensorSettingsWidget)
{
    ui->setupUi(this);

    connect(ui->sendRawCheckBox, SIGNAL(toggled(bool)), mav, SLOT(enableRawSensorDataTransmission(bool)));
    connect(ui->sendControllerCheckBox, SIGNAL(toggled(bool)), mav, SLOT(enableRawControllerDataTransmission(bool)));
    connect(ui->sendExtendedCheckBox, SIGNAL(toggled(bool)), mav, SLOT(enableExtendedSystemStatusTransmission(bool)));
    connect(ui->sendRCCheckBox, SIGNAL(toggled(bool)), mav, SLOT(enableRCChannelDataTransmission(bool)));
    connect(ui->sendPositionCheckBox, SIGNAL(toggled(bool)), mav, SLOT(enablePositionTransmission(bool)));
    connect(ui->sendExtra1CheckBox, SIGNAL(toggled(bool)), mav, SLOT(enableExtra1Transmission(bool)));
    connect(ui->sendExtra2CheckBox, SIGNAL(toggled(bool)), mav, SLOT(enableExtra2Transmission(bool)));
    connect(ui->sendExtra3CheckBox, SIGNAL(toggled(bool)), mav, SLOT(enableExtra3Transmission(bool)));
}

QGCSensorSettingsWidget::~QGCSensorSettingsWidget()
{
    delete ui;
}

void QGCSensorSettingsWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
