#include "QGCCommandButton.h"
#include "ui_QGCCommandButton.h"

#include "MAVLinkProtocol.h"
#include "UASManager.h"

QGCCommandButton::QGCCommandButton(QWidget *parent) :
    QGCToolWidgetItem("CommandButton", parent),
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

    // Add commands to combo box
    ui->editCommandComboBox->addItem("DO: Control Video", MAV_CMD_DO_CONTROL_VIDEO);
    ui->editCommandComboBox->addItem("PREFLIGHT: Calibration", MAV_CMD_PREFLIGHT_CALIBRATION);
}

QGCCommandButton::~QGCCommandButton()
{
    delete ui;
}

void QGCCommandButton::sendCommand()
{
    if (QGCToolWidgetItem::uas)
    {
        // FIXME
        int index = 0;//ui->editCommandComboBox->userData()
        MAV_CMD command = static_cast<MAV_CMD>(index);

        QGCToolWidgetItem::uas->executeCommand(command);
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

void QGCCommandButton::startEditMode()
{
    ui->editCommandComboBox->show();
    ui->editFinishButton->show();
    ui->editNameLabel->show();
    ui->editButtonName->show();
    isInEditMode = true;
}

void QGCCommandButton::endEditMode()
{
    ui->editCommandComboBox->hide();
    ui->editFinishButton->hide();
    ui->editNameLabel->hide();
    ui->editButtonName->hide();

    // Write to settings
    emit editingFinished();

    isInEditMode = false;
}

void QGCCommandButton::writeSettings(QSettings& settings)
{
    settings.setValue("TYPE", "COMMANDBUTTON");
    settings.setValue("QGC_ACTION_BUTTON_DESCRIPTION", ui->nameLabel->text());
    settings.setValue("QGC_ACTION_BUTTON_BUTTONTEXT", ui->commandButton->text());
    settings.setValue("QGC_ACTION_BUTTON_ACTIONID", ui->editCommandComboBox->currentIndex());
    settings.sync();
}

void QGCCommandButton::readSettings(const QSettings& settings)
{
    ui->editNameLabel->setText(settings.value("QGC_ACTION_BUTTON_DESCRIPTION", "ERROR LOADING BUTTON").toString());
    ui->editButtonName->setText(settings.value("QGC_ACTION_BUTTON_BUTTONTEXT", "UNKNOWN").toString());
    ui->editCommandComboBox->setCurrentIndex(settings.value("QGC_ACTION_BUTTON_ACTIONID", 0).toInt());

    ui->nameLabel->setText(settings.value("QGC_ACTION_BUTTON_DESCRIPTION", "ERROR LOADING BUTTON").toString());
    ui->commandButton->setText(settings.value("QGC_ACTION_BUTTON_BUTTONTEXT", "UNKNOWN").toString());
    ui->editCommandComboBox->setCurrentIndex(settings.value("QGC_ACTION_BUTTON_ACTIONID", 0).toInt());
    qDebug() << "DONE READING SETTINGS";
}
