#include "QGCMapToolBar.h"
#include "QGCMapWidget.h"
#include "ui_QGCMapToolBar.h"

QGCMapToolBar::QGCMapToolBar(QWidget *parent) :
    QWidget(parent),
    map(NULL),
    ui(new Ui::QGCMapToolBar)
{
    ui->setupUi(this);
}

void QGCMapToolBar::setMap(QGCMapWidget* map)
{
    this->map = map;

    if (map)
    {
        connect(ui->goToButton, SIGNAL(clicked()), map, SLOT(showGoToDialog()));
        connect(ui->goHomeButton, SIGNAL(clicked()), map, SLOT(goHome()));
        connect(map, SIGNAL(OnTileLoadStart()), this, SLOT(tileLoadStart()));
        connect(map, SIGNAL(OnTileLoadComplete()), this, SLOT(tileLoadEnd()));
        connect(map, SIGNAL(OnTilesStillToLoad(int)), this, SLOT(tileLoadProgress(int)));
        connect(ui->ripMapButton, SIGNAL(clicked()), map, SLOT(cacheVisibleRegion()));

        ui->followCheckBox->setChecked(map->getFollowUAVEnabled());
        connect(ui->followCheckBox, SIGNAL(clicked(bool)), map, SLOT(setFollowUAVEnabled(bool)));

        // Edit mode handling
        ui->editButton->hide();
    }
}

void QGCMapToolBar::tileLoadStart()
{
    ui->posLabel->setText(QString("Starting to load tiles.."));
}

void QGCMapToolBar::tileLoadEnd()
{
    ui->posLabel->setText(QString("Finished"));
}

void QGCMapToolBar::tileLoadProgress(int progress)
{
    if (progress == 1)
    {
        ui->posLabel->setText(QString("1 tile to load.."));
    }
    else if (progress > 0)
    {
        ui->posLabel->setText(QString("%1 tiles to load..").arg(progress));
    }
    else
    {
        tileLoadEnd();
    }
}

QGCMapToolBar::~QGCMapToolBar()
{
    delete ui;
}
