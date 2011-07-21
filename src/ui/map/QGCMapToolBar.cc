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
        connect(ui->lastPosButton, SIGNAL(clicked()), map, SLOT(loadSettings()));
        connect(map, SIGNAL(OnTileLoadStart()), this, SLOT(tileLoadStart()));
        connect(map, SIGNAL(OnTileLoadComplete()), this, SLOT(tileLoadEnd()));
        connect(map, SIGNAL(OnTilesStillToLoad(int)), this, SLOT(tileLoadProgress(int)));
        connect(ui->ripMapButton, SIGNAL(clicked()), map, SLOT(cacheVisibleRegion()));

        ui->followCheckBox->setChecked(map->getFollowUAVEnabled());
        connect(ui->followCheckBox, SIGNAL(clicked(bool)), map, SLOT(setFollowUAVEnabled(bool)));

        // Edit mode handling
        ui->editButton->hide();

//        const int uavTrailTimeList[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};                      // seconds
//        const int uavTrailTimeCount = 10;

//        const int uavTrailDistanceList[] = {1, 2, 5, 10, 20, 50, 100, 200, 500};             // meters
//        const int uavTrailDistanceCount = 9;

//        optionsMenu.setParent(this);


//        // Build up menu
//        //trailPlotMenu(tr("Add trail dot every.."), this);
//        for (int i = 0; i < uavTrailTimeCount; ++i)
//        {
//            trailPlotMenu.addAction(QString("%1 second%2").arg(uavTrailTimeList[i]).arg((uavTrailTimeList[i] > 1) ? "s" : ""), this, SLOT(setUAVTrailTime()));
//        }
//        for (int i = 0; i < uavTrailDistanceCount; ++i)
//        {
//            trailPlotMenu.addAction(QString("%1 meter%2").arg(uavTrailDistanceList[i]).arg((uavTrailDistanceList[i] > 1) ? "s" : ""), this, SLOT(setUAVTrailDistance()));
//        }
//        optionsMenu.addMenu(&trailPlotMenu);

//        ui->optionsButton->setMenu(&optionsMenu);
    }
}

void QGCMapToolBar::setUAVTrailTime()
{

}

void QGCMapToolBar::setUAVTrailDistance()
{

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
