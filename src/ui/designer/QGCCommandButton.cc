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
    ui->editCommandComboBox->setEditable(true);
}

QGCCommandButton::~QGCCommandButton()
{
    delete ui;
}

void QGCCommandButton::sendCommand()
{
    if (QGCToolWidgetItem::uas) {
        // FIXME
        int index = ui->editCommandComboBox->itemData(ui->editCommandComboBox->currentIndex()).toInt();
        MAV_CMD command = static_cast<MAV_CMD>(index);
        int confirm = (ui->editConfirmationCheckBox->isChecked()) ? 1 : 0;
        float param1 = ui->editParam1SpinBox->value();
        float param2 = ui->editParam2SpinBox->value();
        float param3 = ui->editParam3SpinBox->value();
        float param4 = ui->editParam4SpinBox->value();
        int component = ui->editComponentSpinBox->value();

        QGCToolWidgetItem::uas->executeCommand(command, confirm, param1, param2, param3, param4, component);
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
    if (!ui->editParamsVisibleCheckBox->isChecked()) {
        ui->editParam1SpinBox->hide();
        ui->editParam2SpinBox->hide();
        ui->editParam3SpinBox->hide();
        ui->editParam4SpinBox->hide();
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
    settings.setValue("QGC_ACTION_BUTTON_DESCRIPTION", ui->nameLabel->text());
    settings.setValue("QGC_ACTION_BUTTON_BUTTONTEXT", ui->commandButton->text());
    settings.setValue("QGC_ACTION_BUTTON_ACTIONID", ui->editCommandComboBox->itemData(ui->editCommandComboBox->currentIndex()).toInt());
    settings.setValue("QGC_COMMAND_BUTTON_PARAMS_VISIBLE", ui->editParamsVisibleCheckBox->isChecked());
    settings.sync();
}

void QGCCommandButton::readSettings(const QSettings& settings)
{
    ui->editNameLabel->setText(settings.value("QGC_ACTION_BUTTON_DESCRIPTION", "ERROR LOADING BUTTON").toString());
    ui->editButtonName->setText(settings.value("QGC_ACTION_BUTTON_BUTTONTEXT", "UNKNOWN").toString());
    ui->editCommandComboBox->setCurrentIndex(settings.value("QGC_ACTION_BUTTON_ACTIONID", 0).toInt());

    ui->nameLabel->setText(settings.value("QGC_ACTION_BUTTON_DESCRIPTION", "ERROR LOADING BUTTON").toString());
    ui->commandButton->setText(settings.value("QGC_ACTION_BUTTON_BUTTONTEXT", "UNKNOWN").toString());

    int commandId = settings.value("QGC_ACTION_BUTTON_ACTIONID", 0).toInt();
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
    }
    else
    {
        ui->editParam1SpinBox->hide();
        ui->editParam2SpinBox->hide();
        ui->editParam3SpinBox->hide();
        ui->editParam4SpinBox->hide();
    }
    qDebug() << "DONE READING SETTINGS";
}
