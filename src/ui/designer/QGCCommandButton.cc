#include <QDockWidget>
#include <QDebug>

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

    responsecount = 0;
    responsenum = 0;

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

    // Add commands to combo box
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
    ui->editCommandComboBox->addItem("NAV_WAYPOINT", MAV_CMD_NAV_WAYPOINT);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_LOITER_UNLIM", MAV_CMD_NAV_LOITER_UNLIM);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_LOITER_TURNS", MAV_CMD_NAV_LOITER_TURNS);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_LOITER_TIME", MAV_CMD_NAV_LOITER_TIME);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_RETURN_TO_LAUNCH", MAV_CMD_NAV_RETURN_TO_LAUNCH);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_LAND", MAV_CMD_NAV_LAND);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_TAKEOFF", MAV_CMD_NAV_TAKEOFF);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_ROI", MAV_CMD_NAV_ROI);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_PATHPLANNING", MAV_CMD_NAV_PATHPLANNING);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_SPLINE_WAYPOINT", MAV_CMD_NAV_SPLINE_WAYPOINT);
    ui->editCommandComboBox->addItem("MAV_CMD_NAV_GUIDED_ENABLE", MAV_CMD_NAV_GUIDED_ENABLE);
    ui->editCommandComboBox->addItem("MAV_CMD_CONDITION_DELAY", MAV_CMD_CONDITION_DELAY);
    ui->editCommandComboBox->addItem("MAV_CMD_CONDITION_CHANGE_ALT", MAV_CMD_CONDITION_CHANGE_ALT);
    ui->editCommandComboBox->addItem("MAV_CMD_CONDITION_DISTANCE", MAV_CMD_CONDITION_DISTANCE);
    ui->editCommandComboBox->addItem("MAV_CMD_CONDITION_YAW", MAV_CMD_CONDITION_YAW);
    ui->editCommandComboBox->addItem("MAV_CMD_CONDITION_LAST", MAV_CMD_CONDITION_LAST);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_SET_MODE", MAV_CMD_DO_SET_MODE);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_JUMP", MAV_CMD_DO_JUMP);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_CHANGE_SPEED", MAV_CMD_DO_CHANGE_SPEED);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_SET_HOME", MAV_CMD_DO_SET_HOME);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_SET_PARAMETER", MAV_CMD_DO_SET_PARAMETER);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_SET_RELAY", MAV_CMD_DO_SET_RELAY);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_REPEAT_RELAY", MAV_CMD_DO_REPEAT_RELAY);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_SET_SERVO", MAV_CMD_DO_SET_SERVO);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_REPEAT_SERVO", MAV_CMD_DO_REPEAT_SERVO);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_FLIGHTTERMINATION", MAV_CMD_DO_FLIGHTTERMINATION);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_LAND_START", MAV_CMD_DO_LAND_START);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_RALLY_LAND", MAV_CMD_DO_RALLY_LAND);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_GO_AROUND", MAV_CMD_DO_GO_AROUND);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_CONTROL_VIDEO", MAV_CMD_DO_CONTROL_VIDEO);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_SET_ROI", MAV_CMD_DO_SET_ROI);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_DIGICAM_CONFIGURE", MAV_CMD_DO_DIGICAM_CONFIGURE);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_DIGICAM_CONTROL", MAV_CMD_DO_DIGICAM_CONTROL);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_MOUNT_CONFIGURE", MAV_CMD_DO_MOUNT_CONFIGURE);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_MOUNT_CONTROL", MAV_CMD_DO_MOUNT_CONTROL);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_SET_CAM_TRIGG_DIST", MAV_CMD_DO_SET_CAM_TRIGG_DIST);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_FENCE_ENABLE", MAV_CMD_DO_FENCE_ENABLE);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_PARACHUTE", MAV_CMD_DO_PARACHUTE);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_INVERTED_FLIGHT", MAV_CMD_DO_INVERTED_FLIGHT);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_MOUNT_CONTROL_QUAT", MAV_CMD_DO_MOUNT_CONTROL_QUAT);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_GUIDED_MASTER", MAV_CMD_DO_GUIDED_MASTER);
    ui->editCommandComboBox->addItem("MAV_CMD_DO_GUIDED_LIMITS", MAV_CMD_DO_GUIDED_LIMITS);
    ui->editCommandComboBox->addItem("MAV_CMD_PREFLIGHT_CALIBRATION", MAV_CMD_PREFLIGHT_CALIBRATION);
    ui->editCommandComboBox->addItem("MAV_CMD_PREFLIGHT_SET_SENSOR_OFFSETS", MAV_CMD_PREFLIGHT_SET_SENSOR_OFFSETS);
    ui->editCommandComboBox->addItem("MAV_CMD_PREFLIGHT_STORAGE", MAV_CMD_PREFLIGHT_STORAGE);
    ui->editCommandComboBox->addItem("MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN", MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN);
    ui->editCommandComboBox->addItem("MAV_CMD_OVERRIDE_GOTO", MAV_CMD_OVERRIDE_GOTO);
    ui->editCommandComboBox->addItem("MAV_CMD_MISSION_START", MAV_CMD_MISSION_START);
    ui->editCommandComboBox->addItem("MAV_CMD_COMPONENT_ARM_DISARM", MAV_CMD_COMPONENT_ARM_DISARM);
    ui->editCommandComboBox->addItem("MAV_CMD_START_RX_PAIR", MAV_CMD_START_RX_PAIR);
    ui->editCommandComboBox->addItem("MAV_CMD_IMAGE_START_CAPTURE", MAV_CMD_IMAGE_START_CAPTURE);
    ui->editCommandComboBox->addItem("MAV_CMD_IMAGE_STOP_CAPTURE", MAV_CMD_IMAGE_STOP_CAPTURE);
    ui->editCommandComboBox->addItem("MAV_CMD_VIDEO_START_CAPTURE", MAV_CMD_VIDEO_START_CAPTURE);
    ui->editCommandComboBox->addItem("MAV_CMD_VIDEO_STOP_CAPTURE", MAV_CMD_VIDEO_STOP_CAPTURE);
    ui->editCommandComboBox->addItem("MAV_CMD_PANORAMA_CREATE", MAV_CMD_PANORAMA_CREATE);
    ui->editCommandComboBox->addItem("MAV_CMD_PAYLOAD_PREPARE_DEPLOY", MAV_CMD_PAYLOAD_PREPARE_DEPLOY);
    ui->editCommandComboBox->addItem("MAV_CMD_PAYLOAD_CONTROL_DEPLOY", MAV_CMD_PAYLOAD_CONTROL_DEPLOY);
    ui->editCommandComboBox->setEditable(true);

    init();
}

QGCCommandButton::~QGCCommandButton()
{
    delete ui;
}

void QGCCommandButton::sendCommand()
{
    if (QGCToolWidgetItem::uas)
    {
        if (responsenum != 0)
        {
            if (responsecount == 0)
            {
                //We're finished. Reset.
                qDebug() << "Finished sequence";
                QGCToolWidgetItem::uas->executeCommandAck(responsenum-responsecount,true);
                responsecount = responsenum;
                return;
            }
            if (responsecount < responsenum)
            {
                qDebug() << responsecount << responsenum;
                QGCToolWidgetItem::uas->executeCommandAck(responsenum-responsecount,true);
                responsecount--;
                return;
            }
            else
            {
                qDebug() << "No sequence yet, sending command";
                responsecount--;
            }
        }
        // Check if command text is a number
        bool ok;
        int index = 0;
        index = ui->editCommandComboBox->currentText().toInt(&ok);
        if (!ok)
        {
            // Command was not a number, assume it was one of the text items
            index = ui->editCommandComboBox->itemData(ui->editCommandComboBox->currentIndex()).toInt(&ok);
            if (ok)
            {
                // Text item found, proceed
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
                if (showlabelname != "")
                {
                    emit showLabel(showlabelname,index);
                }
                QGCToolWidgetItem::uas->executeCommand(command, confirm, param1, param2, param3, param4, param5, param6, param7, component);
                //qDebug() << __FILE__ << __LINE__ << "SENDING COMMAND" << index;
            }
        }
    }
    else
    {
        qDebug() << __FILE__ << __LINE__ << "NO UAS SET, DOING NOTHING";
    }
}

void QGCCommandButton::setCommandButtonName(QString text)
{
    ui->commandButton->setText(text);
}

void QGCCommandButton::setEditMode(bool editMode)
{
    // Hide elements
    ui->commandButton->setVisible(!editMode);
    ui->nameLabel->setVisible(!editMode);

    ui->editCommandComboBox->blockSignals(!editMode);
    ui->editCommandComboBox->setVisible(editMode);
    ui->editFinishButton->setVisible(editMode);
    ui->editNameLabel->setVisible(editMode);
    ui->editButtonName->setVisible(editMode);
    ui->editConfirmationCheckBox->setVisible(editMode);
    ui->editComponentSpinBox->setVisible(editMode);
    ui->editParamsVisibleCheckBox->setVisible(editMode);
    bool showParams = editMode || ui->editParamsVisibleCheckBox->isChecked();
    ui->editParam1SpinBox->setVisible(showParams);
    ui->editParam2SpinBox->setVisible(showParams);
    ui->editParam3SpinBox->setVisible(showParams);
    ui->editParam4SpinBox->setVisible(showParams);
    ui->editParam5SpinBox->setVisible(showParams);
    ui->editParam6SpinBox->setVisible(showParams);
    ui->editParam7SpinBox->setVisible(showParams);

    ui->editLine1->setVisible(editMode);
    ui->editLine2->setVisible(editMode);

    QGCToolWidgetItem::setEditMode(editMode);
}

void QGCCommandButton::writeSettings(QSettings& settings)
{
    //qDebug() << "COMMAND BUTTON WRITING SETTINGS";
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
void QGCCommandButton::readSettings(const QString& pre,const QVariantMap& settings)
{
    ui->editButtonName->setText(settings.value(pre + "QGC_COMMAND_BUTTON_BUTTONTEXT", "UNKNOWN").toString());
    ui->editCommandComboBox->setCurrentIndex(settings.value(pre + "QGC_COMMAND_BUTTON_COMMANDID", 0).toInt());
    ui->commandButton->setText(settings.value(pre + "QGC_COMMAND_BUTTON_BUTTONTEXT", "UNKNOWN").toString());

    int commandId = settings.value(pre + "QGC_COMMAND_BUTTON_COMMANDID", 0).toInt();

    ui->editParam1SpinBox->setValue(settings.value(pre + "QGC_COMMAND_BUTTON_PARAM1", 0.0).toDouble());
    ui->editParam2SpinBox->setValue(settings.value(pre + "QGC_COMMAND_BUTTON_PARAM2", 0.0).toDouble());
    ui->editParam3SpinBox->setValue(settings.value(pre + "QGC_COMMAND_BUTTON_PARAM3", 0.0).toDouble());
    ui->editParam4SpinBox->setValue(settings.value(pre + "QGC_COMMAND_BUTTON_PARAM4", 0.0).toDouble());
    ui->editParam5SpinBox->setValue(settings.value(pre + "QGC_COMMAND_BUTTON_PARAM5", 0.0).toDouble());
    ui->editParam6SpinBox->setValue(settings.value(pre + "QGC_COMMAND_BUTTON_PARAM6", 0.0).toDouble());
    ui->editParam7SpinBox->setValue(settings.value(pre + "QGC_COMMAND_BUTTON_PARAM7", 0.0).toDouble());

    ui->editCommandComboBox->setCurrentIndex(0);

    // Find combobox entry for this data
    for (int i = 0; i < ui->editCommandComboBox->count(); ++i)
    {
        if (commandId == ui->editCommandComboBox->itemData(i).toInt())
        {
            ui->editCommandComboBox->setCurrentIndex(i);
        }
    }

    ui->editParamsVisibleCheckBox->setChecked(settings.value(pre + "QGC_COMMAND_BUTTON_PARAMS_VISIBLE").toBool());
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

    ui->editNameLabel->setText(settings.value(pre + "QGC_COMMAND_BUTTON_DESCRIPTION", "ERROR LOADING BUTTON").toString());
    ui->nameLabel->setText(settings.value(pre + "QGC_COMMAND_BUTTON_DESCRIPTION", "ERROR LOADING BUTTON").toString());

    responsenum = settings.value(pre + "QGC_COMMAND_BUTTON_RESPONSE",0).toInt();
    responsecount = responsenum;
}
void QGCCommandButton::readSettings(const QSettings& settings)
{
    ui->editButtonName->setText(settings.value("QGC_COMMAND_BUTTON_BUTTONTEXT", "UNKNOWN").toString());
    ui->editCommandComboBox->setCurrentIndex(settings.value("QGC_COMMAND_BUTTON_COMMANDID", 0).toInt());
    ui->commandButton->setText(settings.value("QGC_COMMAND_BUTTON_BUTTONTEXT", "UNKNOWN").toString());

    int commandId = settings.value("QGC_COMMAND_BUTTON_COMMANDID", 0).toInt();

    ui->editParam1SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM1", 0.0).toDouble());
    ui->editParam2SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM2", 0.0).toDouble());
    ui->editParam3SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM3", 0.0).toDouble());
    ui->editParam4SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM4", 0.0).toDouble());
    ui->editParam5SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM5", 0.0).toDouble());
    ui->editParam6SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM6", 0.0).toDouble());
    ui->editParam7SpinBox->setValue(settings.value("QGC_COMMAND_BUTTON_PARAM7", 0.0).toDouble());

    showlabelname = settings.value("QGC_COMMAND_BUTTON_LABEL","").toString();

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

    ui->editNameLabel->setText(settings.value("QGC_COMMAND_BUTTON_DESCRIPTION", "ERROR LOADING BUTTON").toString());
    ui->nameLabel->setText(settings.value("QGC_COMMAND_BUTTON_DESCRIPTION", "ERROR LOADING BUTTON").toString());
    responsenum = settings.value("QGC_COMMAND_BUTTON_RESPONSE",0).toInt();
    responsecount = responsenum;
}
