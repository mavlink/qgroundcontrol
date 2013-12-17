#include "QGCLogDownloader.h"
#include "ui_QGCLogDownloader.h"
#include "UASManager.h"

QGCLogDownloader::QGCLogDownloader(QWidget *parent) :
    QWidget(parent),
    mav(NULL),
    ui(new Ui::QGCLogDownloader)
{
    ui->setupUi(this);

    setActiveUAS(UASManager::instance()->getActiveUAS());

    connect(UASManager::instance(), SIGNAL(UASDeleted(UASInterface*)), this, SLOT(forgetUAS(UASInterface*)));
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

}

QGCLogDownloader::~QGCLogDownloader()
{
    delete ui;
}

/**
 *
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void QGCLogDownloader::setActiveUAS(UASInterface* uas)
{
    if (uas == this->mav)
        return; // no need to rewire

    // Disconnect the previous one (if any)
    forgetUAS(this->mav);

    // Connect this one
    if (uas) {
        connect(ui->downloadButton, SIGNAL(clicked()), uas->getLogManager(), SLOT(requestLogList()));
    }
}

void QGCLogDownloader::forgetUAS(UASInterface* uas)
{
    disconnect(ui->downloadButton, SIGNAL(clicked()), uas->getLogManager(), SLOT(requestLogList()));
}
