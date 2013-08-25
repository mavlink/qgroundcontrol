#include "QGCPX4SensorCalibration.h"
#include "ui_QGCPX4SensorCalibration.h"
#include <UASManager.h>
#include <QMenu>
#include <QScrollBar>

QGCPX4SensorCalibration::QGCPX4SensorCalibration(QWidget *parent) :
    QWidget(parent),
    activeUAS(NULL),
    clearAction(new QAction(tr("Clear Text"), this)),
    ui(new Ui::QGCPX4SensorCalibration)
{
    ui->setupUi(this);
    connect(clearAction, SIGNAL(triggered()), ui->textView, SLOT(clear()));

    setInstructionImage("./files/images/px4/calibration/accel_z-.png");

    setObjectName("PX4_SENSOR_CALIBRATION");

    setStyleSheet("QScrollArea { border: 0px; } QPlainTextEdit { border: 0px }");

    setActiveUAS(UASManager::instance()->getActiveUAS());
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
}

QGCPX4SensorCalibration::~QGCPX4SensorCalibration()
{
    delete ui;
}

void QGCPX4SensorCalibration::setInstructionImage(const QString &path)
{
    instructionIcon.load(path);

    int w = ui->iconLabel->width();
    int h = ui->iconLabel->height();

    ui->iconLabel->setPixmap(instructionIcon.scaled(w, h, Qt::KeepAspectRatio));
}

void QGCPX4SensorCalibration::resizeEvent(QResizeEvent* event)
{

    int w = ui->iconLabel->width();
    int h = ui->iconLabel->height();

    ui->iconLabel->setPixmap(instructionIcon.scaled(w, h, Qt::KeepAspectRatio));

    QWidget::resizeEvent(event);
}

void QGCPX4SensorCalibration::setActiveUAS(UASInterface* uas)
{
    if (!uas)
        return;

    if (activeUAS) {
        disconnect(uas, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(handleTextMessage(int,int,int,QString)));
        ui->textView->clear();
    }

    connect(uas, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(handleTextMessage(int,int,int,QString)));
    activeUAS = uas;
}

void QGCPX4SensorCalibration::handleTextMessage(int uasid, int compId, int severity, QString text)
{
    // XXX color messages according to severity

    QPlainTextEdit *msgWidget = ui->textView;

    //turn off updates while we're appending content to avoid breaking the autoscroll behavior
    msgWidget->setUpdatesEnabled(false);
    QScrollBar *scroller = msgWidget->verticalScrollBar();

    UASInterface *uas = UASManager::instance()->getUASForId(uasid);
    QString uasName(uas->getUASName());
    QString colorName(uas->getColor().name());
    //change styling based on severity
    if (160 == severity ) { //TODO where is the constant for "critical" severity?
        //GAudioOutput::instance()->say(text.toLower());
        msgWidget->appendHtml(QString("<p style=\"color:#DC143C;background-color:#FFFACD;font-size:large;font-weight:bold\">[%1:%2] %3</p>").arg(uasName).arg(compId).arg(text));
    }
    else {
        msgWidget->appendHtml(QString("<p style=\"color:%1;font-size:smaller\">[%2:%3] %4</p>").arg(colorName).arg(uasName).arg(compId).arg(text));
    }

    // Ensure text area scrolls correctly
    scroller->setValue(scroller->maximum());
    msgWidget->setUpdatesEnabled(true);

}

void QGCPX4SensorCalibration::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.addAction(clearAction);
    menu.exec(event->globalPos());
}
