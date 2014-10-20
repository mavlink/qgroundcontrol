#include <QMessageBox>

#include "boardwidget.h"
#include "ui_boardwidget.h"

BoardWidget::BoardWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::boardWidget)
{
    ui->setupUi(this);

    connect(ui->flashButton, SIGNAL(clicked()), this, SLOT(flashFirmware()));
    connect(ui->cancelButton, SIGNAL(clicked()), this, SIGNAL(cancelFirmwareUpload()));

    setBoardImage("./files/images/px4/calibration/accel_z-.png");
}

BoardWidget::~BoardWidget()
{
    delete ui;
}

void BoardWidget::setBoardImage(const QString &path)
{
    boardIcon.load(path);

    int w = ui->iconLabel->width();
    int h = ui->iconLabel->height();

    ui->iconLabel->setPixmap(boardIcon.scaled(w, h, Qt::KeepAspectRatio));
}

void BoardWidget::resizeEvent(QResizeEvent* event)
{

    int w = ui->iconLabel->width();
    int h = ui->iconLabel->height();

    ui->iconLabel->setPixmap(boardIcon.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    QWidget::resizeEvent(event);
}

void BoardWidget::flashFirmware()
{
    QString url = ui->firmwareComboBox->itemData(ui->firmwareComboBox->currentIndex()).toString();

    if (url.contains("beta")) {
        int ret = QMessageBox::warning(this, tr("WARNING: BETA FIRMWARE"),
                                       tr("This firmware version is ONLY intended for beta testers."
                                          "Although it has received FLIGHT TESTING, it represents\n"
                                          "actively changed code. Do NOT use for normal operation.\n"
                                          "Disclaimer of the BSD open source license as redistributed with source code applies."),
                                       QMessageBox::Ok
                                       | QMessageBox::Cancel,
                                       QMessageBox::Cancel);
        if (ret != QMessageBox::Ok)
            return;
    }

    if (url.contains("continuous")) {
        int ret = QMessageBox::critical(this, tr("WARNING: CONTINUOUS BUILD FIRMWARE"),
                                       tr("This firmware has NOT BEEN FLIGHT TESTED.\n"
                                          "It is only intended for DEVELOPERS. Run bench tests\n"
                                          "without props first, do NOT fly this without addional\n"
                                          "safety precautions. Follow the mailing list actively when using it.\n"
                                          "Disclaimer of the BSD open source license as redistributed with source code applies."),
                                       QMessageBox::Ok
                                       | QMessageBox::Cancel,
                                       QMessageBox::Cancel);
        if (ret != QMessageBox::Ok)
            return;
    }

    if (url.contains("stable")) {
        int ret = QMessageBox::information(this, tr("Flashing stable build"),
                                       tr("By flashing this firmware you agree to the terms and\n"
                                          "disclaimer of the BSD open source license, as\n"
                                          "redistributed with the source code."),
                                       QMessageBox::Ok
                                       | QMessageBox::Cancel,
                                       QMessageBox::Cancel);
        if (ret != QMessageBox::Ok)
            return;
    }

    emit flashFirmwareURL(url);
}

void BoardWidget::updateStatus(const QString &status)
{
    ui->statusLabel->setText(status);
}

void BoardWidget::setBoardInfo(int board_id, const QString &boardName, const QString &bootLoader)
{
    // XXX this should not be hardcoded
    ui->firmwareComboBox->clear();

    ui->boardNameLabel->setText(boardName);
    ui->bootloaderLabel->setText(bootLoader);

    switch (board_id) {
    case 5:
    {   
        setBoardImage(":/files/boards/px4fmu_1.x.png");
        ui->firmwareComboBox->addItem("Standard Version (stable)", "http://px4.oznet.ch/stable/px4fmu-v1_default.px4");
        ui->firmwareComboBox->addItem("Beta Testing (beta)", "http://px4.oznet.ch/beta/px4fmu-v1_default.px4");
        ui->firmwareComboBox->addItem("Developer Build (master)", "http://px4.oznet.ch/continuous/px4fmu-v1_default.px4");
    }
        break;
    case 6:
    {
        setBoardImage(":/files/boards/px4flow_1.x.png");
        ui->firmwareComboBox->addItem("Standard Version (stable)", "http://px4.oznet.ch/stable/px4flow.px4");
        ui->firmwareComboBox->addItem("Beta Testing (beta)", "http://px4.oznet.ch/beta/px4flow.px4");
        ui->firmwareComboBox->addItem("Developer Build (master)", "http://px4.oznet.ch/continuous/px4flow.px4");
    }
        break;
    case 9:
        setBoardImage(":/files/boards/px4fmu_2.x.png");
        ui->firmwareComboBox->addItem("Standard Version (stable)", "http://px4.oznet.ch/stable/px4fmu-v2_default.px4");
        ui->firmwareComboBox->addItem("Beta Testing (beta)", "http://px4.oznet.ch/beta/px4fmu-v2_default.px4");
        ui->firmwareComboBox->addItem("Developer Build (master)", "http://px4.oznet.ch/continuous/px4fmu-v2_default.px4");
        break;

    }

}
