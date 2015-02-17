#include "QGCMapToolBar.h"
#include "QGCMapWidget.h"
#include "ui_QGCMapToolBar.h"

QGCMapToolBar::QGCMapToolBar(QWidget *parent) :
    QWidget(parent),
    _ui(new Ui::QGCMapToolBar),
    _map(NULL),
    _optionsMenu(new QMenu(this)),
    _trailPlotMenu(new QMenu(this)),
    _updateTimesMenu(new QMenu(this)),
    _mapTypesMenu(new QMenu(this)),
    _trailSettingsGroup(new QActionGroup(this)),
    _updateTimesGroup(new QActionGroup(this)),
    _mapTypesGroup(new QActionGroup(this)),
    _statusMaxLen(15)
{
    _ui->setupUi(this);
}

void QGCMapToolBar::setMap(QGCMapWidget* map)
{
    _map = map;

    if (_map)
    {
        connect(_ui->goToButton, SIGNAL(clicked()), _map, SLOT(showGoToDialog()));
        connect(_ui->goHomeButton, SIGNAL(clicked()), _map, SLOT(goHome()));
        connect(_ui->lastPosButton, SIGNAL(clicked()), _map, SLOT(loadSettings()));
        connect(_ui->clearTrailsButton, SIGNAL(clicked()), _map, SLOT(deleteTrails()));
        connect(_ui->lockCheckBox, SIGNAL(clicked(bool)), _map, SLOT(setZoomBlocked(bool)));
        connect(_map, SIGNAL(OnTileLoadStart()), this, SLOT(tileLoadStart()));
        connect(_map, SIGNAL(OnTileLoadComplete()), this, SLOT(tileLoadEnd()));
        connect(_map, SIGNAL(OnTilesStillToLoad(int)), this, SLOT(tileLoadProgress(int)));
        connect(_ui->ripMapButton, SIGNAL(clicked()), _map, SLOT(cacheVisibleRegion()));

        _ui->followCheckBox->setChecked(_map->getFollowUAVEnabled());
        connect(_ui->followCheckBox, SIGNAL(clicked(bool)), _map, SLOT(setFollowUAVEnabled(bool)));

        // Edit mode handling
        _ui->editButton->hide();

        const int uavTrailTimeList[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};                      // seconds
        const int uavTrailTimeCount = 10;

        const int uavTrailDistanceList[] = {1, 2, 5, 10, 20, 50, 100, 200, 500};             // meters
        const int uavTrailDistanceCount = 9;

        // Set exclusive items
        _trailSettingsGroup->setExclusive(true);
        _updateTimesGroup->setExclusive(true);
        _mapTypesGroup->setExclusive(true);

        // Build up menu
        _trailPlotMenu->setTitle(tr("&Add trail dot every.."));
        _updateTimesMenu->setTitle(tr("&Limit map view update rate to.."));
        _mapTypesMenu->setTitle(tr("&Map type"));


        //setup the mapTypesMenu
        QAction* action;
        action =  _mapTypesMenu->addAction(tr("Bing Hybrid"),this,SLOT(setMapType()));
        action->setData(MapType::BingHybrid);
        action->setCheckable(true);
#ifdef MAP_DEFAULT_TYPE_BING
        action->setChecked(true);
#endif
        _mapTypesGroup->addAction(action);

        action =  _mapTypesMenu->addAction(tr("Google Hybrid"),this,SLOT(setMapType()));
        action->setData(MapType::GoogleHybrid);
        action->setCheckable(true);
#ifdef MAP_DEFAULT_TYPE_GOOGLE
        action->setChecked(true);
#endif
        _mapTypesGroup->addAction(action);

        action =  _mapTypesMenu->addAction(tr("OpenStreetMap"),this,SLOT(setMapType()));
        action->setData(MapType::OpenStreetMap);
        action->setCheckable(true);
#ifdef MAP_DEFAULT_TYPE_OSM
        action->setChecked(true);
#endif
        _mapTypesGroup->addAction(action);

        _optionsMenu->addMenu(_mapTypesMenu);


        // FIXME MARK CURRENT VALUES IN MENU
        QAction *defaultTrailAction = _trailPlotMenu->addAction(tr("No trail"), this, SLOT(setUAVTrailTime()));
        defaultTrailAction->setData(-1);
        defaultTrailAction->setCheckable(true);
        _trailSettingsGroup->addAction(defaultTrailAction);

        for (int i = 0; i < uavTrailTimeCount; ++i)
        {
            action = _trailPlotMenu->addAction(tr("%1 second%2").arg(uavTrailTimeList[i]).arg((uavTrailTimeList[i] > 1) ? "s" : ""), this, SLOT(setUAVTrailTime()));
            action->setData(uavTrailTimeList[i]);
            action->setCheckable(true);
            _trailSettingsGroup->addAction(action);
            if (static_cast<mapcontrol::UAVTrailType::Types>(map->getTrailType()) == mapcontrol::UAVTrailType::ByTimeElapsed && _map->getTrailInterval() == uavTrailTimeList[i])
            {
                // This is the current active time, set the action checked
                action->setChecked(true);
            }
        }
        for (int i = 0; i < uavTrailDistanceCount; ++i)
        {
            action = _trailPlotMenu->addAction(tr("%1 meter%2").arg(uavTrailDistanceList[i]).arg((uavTrailDistanceList[i] > 1) ? "s" : ""), this, SLOT(setUAVTrailDistance()));
            action->setData(uavTrailDistanceList[i]);
            action->setCheckable(true);
            _trailSettingsGroup->addAction(action);
            if (static_cast<mapcontrol::UAVTrailType::Types>(_map->getTrailType()) == mapcontrol::UAVTrailType::ByDistance && _map->getTrailInterval() == uavTrailDistanceList[i])
            {
                // This is the current active time, set the action checked
                action->setChecked(true);
            }
        }

        // Set no trail checked if no action is checked yet
        if (!_trailSettingsGroup->checkedAction())
        {
            defaultTrailAction->setChecked(true);
        }

        _optionsMenu->addMenu(_trailPlotMenu);

        // Add update times menu
        for (int i = 100; i < 5000; i+=400)
        {
            float time = i/1000.0f; // Convert from ms to seconds
            QAction* action = _updateTimesMenu->addAction(tr("%1 seconds").arg(time), this, SLOT(setUpdateInterval()));
            action->setData(time);
            action->setCheckable(true);
            if (time == _map->getUpdateRateLimit())
            {
                action->blockSignals(true);
                action->setChecked(true);
                action->blockSignals(false);
            }
            _updateTimesGroup->addAction(action);
        }

        // If the current time is not part of the menu defaults
        // still add it as new option
        if (!_updateTimesGroup->checkedAction())
        {
            float time = _map->getUpdateRateLimit();
            QAction* action = _updateTimesMenu->addAction(tr("uptate every %1 seconds").arg(time), this, SLOT(setUpdateInterval()));
            action->setData(time);
            action->setCheckable(true);
            action->setChecked(true);
            _updateTimesGroup->addAction(action);
        }
        _optionsMenu->addMenu(_updateTimesMenu);

        _ui->optionsButton->setMenu(_optionsMenu);
    }
}

void QGCMapToolBar::setUAVTrailTime()
{
    QObject* sender = QObject::sender();
    QAction* action = qobject_cast<QAction*>(sender);

    if (action)
    {
        bool ok;
        int trailTime = action->data().toInt(&ok);
        if (ok)
        {
            (_map->setTrailModeTimed(trailTime));
            setStatusLabelText(tr("Trail mode: Every %1 second%2").arg(trailTime).arg((trailTime > 1) ? "s" : ""));
        }
    }
}

void QGCMapToolBar::setStatusLabelText(const QString &text)
{
    _ui->posLabel->setText(text.leftJustified(_statusMaxLen, QChar('.'), true));
}

void QGCMapToolBar::setUAVTrailDistance()
{
    QObject* sender = QObject::sender();
    QAction* action = qobject_cast<QAction*>(sender);

    if (action)
    {
        bool ok;
        int trailDistance = action->data().toInt(&ok);
        if (ok)
        {
            _map->setTrailModeDistance(trailDistance);
            setStatusLabelText(tr("Trail mode: Every %1 meter%2").arg(trailDistance).arg((trailDistance == 1) ? "s" : ""));
        }
    }
}

void QGCMapToolBar::setUpdateInterval()
{
    QObject* sender = QObject::sender();
    QAction* action = qobject_cast<QAction*>(sender);

    if (action)
    {
        bool ok;
        float time = action->data().toFloat(&ok);
        if (ok)
        {
            _map->setUpdateRateLimit(time);
            setStatusLabelText(tr("Limit: %1 second%2").arg(time).arg((time != 1.0f) ? "s" : ""));
        }
    }
}

void QGCMapToolBar::setMapType()
{
    QObject* sender = QObject::sender();
    QAction* action = qobject_cast<QAction*>(sender);

    if (action)
    {
        bool ok;
        int mapType = action->data().toInt(&ok);
        if (ok)
        {
            _map->SetMapType((MapType::Types)mapType);
            setStatusLabelText(tr("Map: %1").arg(mapType));
        }
    }
}

void QGCMapToolBar::tileLoadStart()
{
    setStatusLabelText(tr("Loading"));
}

void QGCMapToolBar::tileLoadEnd()
{
    setStatusLabelText(tr("Finished"));
}

void QGCMapToolBar::tileLoadProgress(int progress)
{
    if (progress == 1)
    {
        setStatusLabelText(tr("1 tile"));
    }
    else if (progress > 0)
    {
        setStatusLabelText(tr("%1 tile").arg(progress));
    }
    else
    {
        tileLoadEnd();
    }
}
