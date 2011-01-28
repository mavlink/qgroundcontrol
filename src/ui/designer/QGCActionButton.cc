#include "QGCActionButton.h"
#include "ui_QGCActionButton.h"

#include "MAVLinkProtocol.h"
#include "UASManager.h"

const char* kActionLabels[MAV_ACTION_NB] =
{"HOLD",
 "START MOTORS",
 "LAUNCH",
 "RETURN",
 "EMERGENCY LAND",
 "EMERGENCY KILL",
 "CONFIRM KILL",
 "CONTINUE",
 "STOP MOTORS",
 "HALT",
 "SHUTDOWN",
 "REBOOT",
 "SET MANUAL",
 "SET AUTO",
 "READ STORAGE",
 "WRITE STORAGE",
 "CALIBRATE RC",
 "CALIBRATE GYRO",
 "CALIBRATE MAG",
 "CALIBRATE ACC",
 "CALIBRATE PRESSURE",
 "START REC",
 "PAUSE REC",
 "STOP REC",
 "TAKEOFF",
 "NAVIGATE",
 "LAND",
 "LOITER",
 "SET ORIGIN",
 "RELAY ON",
 //"RELAY OFF",
 //"GET IMAGE",
 //"START VIDEO",
 //"STOP VIDEO",
 "RESET MAP",
 "RESET PLAN"};

QGCActionButton::QGCActionButton(QWidget *parent) :
    QGCToolWidgetItem("Button", parent),
    ui(new Ui::QGCActionButton),
    uas(NULL)
{
    ui->setupUi(this);

    connect(ui->actionButton, SIGNAL(clicked()), this, SLOT(sendAction()));
    connect(ui->editFinishButton, SIGNAL(clicked()), this, SLOT(endEditMode()));
    connect(ui->editButtonName, SIGNAL(textChanged(QString)), this, SLOT(setActionButtonName(QString)));
    connect(ui->editActionComboBox, SIGNAL(currentIndexChanged(QString)), ui->nameLabel, SLOT(setText(QString)));

    // Hide all edit items
    ui->editActionComboBox->hide();
    ui->editActionsRefreshButton->hide();
    ui->editFinishButton->hide();
    ui->editNameLabel->hide();
    ui->editButtonName->hide();

    // add action labels to combobox
    for (int i = 0; i < MAV_ACTION_NB; i++)
    {
        ui->editActionComboBox->addItem(kActionLabels[i]);
    }
}

QGCActionButton::~QGCActionButton()
{
    delete ui;
}

void QGCActionButton::sendAction()
{
    if (QGCToolWidgetItem::uas)
    {
        MAV_ACTION action = static_cast<MAV_ACTION>(
                ui->editActionComboBox->currentIndex());

        QGCToolWidgetItem::uas->setAction(action);
    }
    else
    {
        qDebug() << __FILE__ << __LINE__ << "NO UAS SET, DOING NOTHING";
    }
}

void QGCActionButton::setActionButtonName(QString text)
{
    ui->actionButton->setText(text);
}

void QGCActionButton::startEditMode()
{
    ui->editActionComboBox->show();
    ui->editActionsRefreshButton->show();
    ui->editFinishButton->show();
    ui->editNameLabel->show();
    ui->editButtonName->show();
    isInEditMode = true;
}

void QGCActionButton::endEditMode()
{
    ui->editActionComboBox->hide();
    ui->editActionsRefreshButton->hide();
    ui->editFinishButton->hide();
    ui->editNameLabel->hide();
    ui->editButtonName->hide();

    // Write to settings
    emit editingFinished();

    isInEditMode = false;
}

void QGCActionButton::writeSettings(QSettings& settings)
{
    settings.setValue("TYPE", "BUTTON");
    settings.setValue("QGC_ACTION_BUTTON_DESCRIPTION", ui->nameLabel->text());
    settings.setValue("QGC_ACTION_BUTTON_BUTTONTEXT", ui->actionButton->text());
    settings.setValue("QGC_ACTION_BUTTON_ACTIONID", ui->editActionComboBox->currentIndex());
    settings.sync();
}

void QGCActionButton::readSettings(const QSettings& settings)
{
    ui->editNameLabel->setText(settings.value("QGC_ACTION_BUTTON_DESCRIPTION", "ERROR LOADING BUTTON").toString());
    ui->editButtonName->setText(settings.value("QGC_ACTION_BUTTON_BUTTONTEXT", "UNKNOWN").toString());
    ui->editActionComboBox->setCurrentIndex(settings.value("QGC_ACTION_BUTTON_ACTIONID", 0).toInt());

    ui->nameLabel->setText(settings.value("QGC_ACTION_BUTTON_DESCRIPTION", "ERROR LOADING BUTTON").toString());
    ui->actionButton->setText(settings.value("QGC_ACTION_BUTTON_BUTTONTEXT", "UNKNOWN").toString());
    ui->editActionComboBox->setCurrentIndex(settings.value("QGC_ACTION_BUTTON_ACTIONID", 0).toInt());
    qDebug() << "DONE READING SETTINGS";
}
