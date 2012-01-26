#include "QGCCommandButton.h"
#include "ui_QGCCommandButton.h"

#include "MAVLinkProtocol.h"
#include "UASManager.h"

QGCCommandButton::QGCCommandButton(QWidget *parent) :
    QGCToolWidgetItem("Command Button", parent),
    ui(new Ui::QGCCommandButton),
    uas(NULL)
{
    ui->setupUi(this);

    connect(ui->commandButton, SIGNAL(clicked()), this, SLOT(sendCommand()));
    connect(ui->editFinishButton, SIGNAL(clicked()), this, SLOT(endEditMode()));
    connect(ui->editButtonName, SIGNAL(textChanged(QString)), this, SLOT(setCommandButtonName(QString)));
    connect(ui->editCommandComboBox, SIGNAL(currentIndexChanged(QString)), ui->nameLabel, SLOT(setText(QString)));

    // Hide all edit items
    ui->editCommandComboBox->hide();
    ui->editFinishButton->hide();
    ui->editNameLabel->hide();
    ui->editButtonName->hide();
    ui->editConfirmationCheckBox->hide();
    ui->editComponentSpinBox->hide();
    ui->editParamsVisibleCheckBox->hide();
    ui->editParam1SpinBox->hide();
    ui->editParam2SpinBox->hide();
    ui->editParam3SpinBox->hide();
    ui->editParam4SpinBox->hide();
    ui->editLine1->hide();
    ui->editLine2->hide();

    ui->editLine1->setStyleSheet("QWidget { border: 1px solid #66666B; border-radius: 3px; padding: 10px 0px 0px 0px; background: #111122; }");
    ui->editLine2->setStyleSheet("QWidget { border: 1px solid #66666B; border-radius: 3px; padding: 10px 0px 0px 0px; background: #111122; }");

    // Add commands to combo box
    ui->editCommandComboBox->addItem("DO: Control Video", MAV_CMD_DO_CONTROL_VIDEO);
    ui->editCommandComboBox->addItem("PREFLIGHT: Calibration", MAV_CMD_PREFLIGHT_CALIBRATION);
    ui->editCommandComboBox->addItem("CUSTOM 0", 0);
    ui->editCommandComboBox->addItem("CUSTOM 1", 1);
    ui->editCommandComboBox->addItem("CUSTOM 2", 2);
    ui->editCommandComboBox->addItem("CUSTOM 3", 3);
    ui->editCommandComboBox->addItem("CUSTOM 4", 4);
    ui->editCommandComboBox->addItem("CUSTOM 5", 5);
    ui->editCommandComboBox->addItem("CUSTOM 6", 6);
    ui->editCommandComboBox->addItem("CUSTOM 7", 7);
    ui->editCommandComboBox->addItem("CUSTOM 8", 8);
    ui->editCommandComboBox->addItem("CUSTOM 9", 9);
    ui->editCommandComboBox->addItem("CUSTOM 10", 10);
    ui->editCommandComboBox->addItem("CUSTOM 11", 11);
    ui->editCommandComboBox->addItem("CUSTOM 12", 12);
    ui->editCommandComboBox->addItem("CUSTOM 13", 13);
    ui->editCommandComboBox->addItem("CUSTOM 14", 14);
    ui->editCommandComboBox->addItem("CUSTOM 15", 15);
    ui->editCommandComboBox->addItem("NAV_WAYPOINT", 16);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_LOITER_UNLIM", 17);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_LOITER_TURNS", 18);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_LOITER_TIME", 19);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_RETURN_TO_LAUNCH", 20);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_LAND", 21);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_TAKEOFF", 22);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_ROI", 80);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_PATHPLANNING", 81);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_SET_MODE", 176);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_CHANGE_SPEED", 178);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_SET_HOME", 179);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_SET_RELAY", 181);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_REPEAT_RELAY", 182);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_SET_SERVO", 183);
    ui->editCommandComboBox->setEditable(true);
}

QGCCommandButton::~QGCCommandButton()
{
    delete ui;
}

void QGCCommandButton::sendCommand()
{
    if (QGCToolWidgetItem::uas) {
        int index = ui->editCommandComboBox->itemData(ui->editCommandComboBox->currentIndex()).toInt();
        MAV_CMD command = static_cast<MAV_CMD>(index);
        int confirm = (ui->editConfirmationCheckBox->isChecked()) ? 1 : 0;
        float param1 = ui->editParam1SpinBox->value();
        float param2 = ui->editParam2SpinBox->value();
        float param3 = ui->editParam3SpinBox->value();
        float param4 = ui->editParam4SpinBox->value();
        float param5 = ui->editParam5SpinBox->value();
        float param6 = ui->editParam6SpinBox->value();
        float param7 = ui->editParam7SpinBox->value();
        int component = ui->editComponentSpinBox->value();

        QGCToolWidgetItem::uas->executeCommand(command, confirm, param1, param2, param3, param4, param5, param6, param7, component);
        qDebug() << __FILE__ << __LINE__ << "SENDING COMMAND" << index;
    } else {
        qDebug() << __FILE__ << __LINE__ << "NO UAS SET, DOING NOTHING";
    }
}

void QGCCommandButton::setCommandButtonName(QString text)
{
    ui->commandButton->setText(text);
}

void QGCCommandButton::startEditMode()
{
    // Hide elements
    ui->commandButton->hide();
    ui->nameLabel->hide();

    ui->editCommandComboBox->show();
    ui->editFinishButton->show();
    ui->editNameLabel->show();
    ui->editButtonName->show();
    ui->editConfirmationCheckBox->show();
    ui->editComponentSpinBox->show();
    ui->editParamsVisibleCheckBox->show();
    ui->editParam1SpinBox->show();
    ui->editParam2SpinBox->show();
    ui->editParam3SpinBox->show();
    ui->editParam4SpinBox->show();
    ui->editParam5SpinBox->show();
    ui->editParam6SpinBox->show();
    ui->editParam7SpinBox->show();
    ui->editLine1->show();
    ui->editLine2->show();
    //setStyleSheet("QGroupBox { border: 1px solid #66666B; border-radius: 3px; padding: 10px 0px 0px 0px; background: #111122; }");
    isInEditMode = true;
}

void QGCCommandButton::endEditMode()
{
    ui->editCommandComboBox->hide();
    ui->editFinishButton->hide();
    ui->editNameLabel->hide();
    ui->editButtonName->hide();
    ui->editConfirmationCheckBox->hide();
    ui->editComponentSpinBox->hide();
    ui->editParamsVisibleCheckBox->hide();
    ui->editLine1->hide();
    ui->editLine2->hide();
    if (!ui->editParamsVisibleCheckBox->isChecked())
    {
        ui->editParam1SpinBox->hide();
        ui->editParam2SpinBox->hide();
        ui->editParam3SpinBox->hide();
        ui->editParam4SpinBox->hide();
        ui->editParam5SpinBox->hide();
        ui->editParam6SpinBox->hide();
        ui->editParam7SpinBox->hide();
    }

    ui->commandButton->show();
    ui->nameLabel->show();

    // Write to settings
    emit editingFinished();
    //setStyleSheet("");
    isInEditMode = false;
}

void QGCCommandButton::writeSettings(QSettings& settings)
{
    qDebug() << "COMMAND BUTTON WRITING SETTINGS";
    settings.setValue("TYPE", "COMMANDBUTTON");
    settings.setValue("QGC_COMMAND_BUTTON_DESCRIPTION", ui->nameLabel->text());
    settings.setValue("QGC_COMMAND_BUTTON_BUTTONTEXT", ui->commandButton->text());
    settings.setValue("QGC_COMMAND_BUTTON_COMMANDID", ui->editCommandComboBox->itemData(ui->editCommandComboBox->currentIndex()).toInt());
    settings.setValue("QGC_COMMAND_BUTTON_PARAMS_VISIBLE", ui->editParamsVisibleCheckBox->isChecked());
    settings.setValue("QGC_COMMAND_BUTTON_PARAM1",  ui->editParam1SpinBox->value());
    settings.setValue("QGC_COMMAND_BUTTON_PARAM2",  ui->editParam2SpinBox->value());
    settings.setValue("QGC_COMMAND_BUTTON_PARAM3",  ui->editParam3SpinBox->value());
    settings.setValue("QGC_COMMAND_BUTTON_PARAM4",  ui->editParam4SpinBox->value());
    settings.setValue("QGC_COMMAND_BUTTON_PARAM5",  ui->editParam5SpinBox->value());
    settings.setValue("QGC_COMMAND_BUTTON_PARAM6",  ui->editParam6SpinBox->value());
    settings.setValue("QGC_COMMAND_BUTTON_PARAM7",  ui->editParam7SpinBox->value());
    settings.sync();
}

void QGCCommandButton::readSettings(const QSettings& settings)
{
    ui->editNameLabel->setText(settings.value("QGC_COMMAND_BUTTON_DESCRIPTION", "ERROR LOADING BUTTON").toString());
    ui->editButtonName->setText(settings.value("QGC_COMMAND_BUTTON_BUTTONTEXT", "UNKNOWN").toString());
    ui->editCommandComboBox->setCurrentIndex(settings.value("QGC_COMMAND_BUTTON_COMMANDID", 0).toInt());

    ui->nameLabel->setText(settings.value("QGC_COMMAND_BUTTON_DESCRIPTION", "ERROR LOADING BUTTON").toString());
    ui->commandButton->setText(settings.value("QGC_COMMAND_BUTTON_BUTTONTEXT", "UNKNOWN").toString());

    int commandId = settings.value("QGC_COMMAND_BUTTON_COMMANDID", 0).toInt();

    ui->editParam1SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM1", 0.0).toDouble());
    ui->editParam2SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM2", 0.0).toDouble());
    ui->editParam3SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM3", 0.0).toDouble());
    ui->editParam4SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM4", 0.0).toDouble());
    ui->editParam5SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM5", 0.0).toDouble());
    ui->editParam6SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM6", 0.0).toDouble());
    ui->editParam7SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM7", 0.0).toDouble());

    ui->editCommandComboBox->setCurrentIndex(0);

    // Find combobox entry for this data
    for (int i = 0; i < ui->editCommandComboBox->count(); ++i)
    {
        if (commandId == ui->editCommandComboBox->itemData(i).toInt())
        {
            ui->editCommandComboBox->setCurrentIndex(i);
        }
    }

    ui->editParamsVisibleCheckBox->setChecked(settings.value("QGC_COMMAND_BUTTON_PARAMS_VISIBLE").toBool());
    if (ui->editParamsVisibleCheckBox->isChecked())
    {
        ui->editParam1SpinBox->show();
        ui->editParam2SpinBox->show();
        ui->editParam3SpinBox->show();
        ui->editParam4SpinBox->show();
        ui->editParam5SpinBox->show();
        ui->editParam6SpinBox->show();
        ui->editParam7SpinBox->show();
    }
    else
    {
        ui->editParam1SpinBox->hide();
        ui->editParam2SpinBox->hide();
        ui->editParam3SpinBox->hide();
        ui->editParam4SpinBox->hide();
        ui->editParam5SpinBox->hide();
        ui->editParam6SpinBox->hide();
        ui->editParam7SpinBox->hide();
    }
    qDebug() << "DONE READING SETTINGS";
}
