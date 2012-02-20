#include "ViewParamWidget.h"

#include <osg/LineWidth>
#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>

#include "UASInterface.h"

ViewParamWidget::ViewParamWidget(GlobalViewParamsPtr& globalViewParams,
                                 QMap<int, SystemViewParamsPtr>& systemViewParamMap,
                                 QWidget* parent, QWidget* mainWindow)
 : QDockWidget(tr("View Parameters"), mainWindow)
 , mGlobalViewParams(globalViewParams)
 , mSystemViewParamMap(systemViewParamMap)
 , mParent(parent)
 , mFollowCameraComboBox(new QComboBox(this))
 , mTabWidget(new QTabWidget(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    QWidget* widget = new QWidget;
    widget->setLayout(layout);

    setWidget(widget);

    buildLayout(layout);

    mTabWidget->setFocusPolicy(Qt::NoFocus);

    connect(parent, SIGNAL(systemCreatedSignal(UASInterface*)),
            this, SLOT(systemCreated(UASInterface*)));
}

void
ViewParamWidget::setFollowCameraId(int id)
{
    for (int i = 0; i < mFollowCameraComboBox->count(); ++i)
    {
        if (mFollowCameraComboBox->itemText(i).endsWith(QString::number(id)))
        {
            mFollowCameraComboBox->setCurrentIndex(i);
            return;
        }
    }

    mFollowCameraComboBox->setCurrentIndex(0);
}

void
ViewParamWidget::systemCreated(UASInterface *uas)
{
    addTab(uas->getUASID());

    QString text("MAV ");
    text += QString::number(uas->getUASID());
    mFollowCameraComboBox->addItem(text);
}

void
ViewParamWidget::setpointsCheckBoxToggled(int state)
{
    if (state == Qt::Checked)
    {
        mSetpointHistoryLengthSpinBox->setEnabled(true);
    }
    else
    {
        mSetpointHistoryLengthSpinBox->setEnabled(false);
    }
}

void
ViewParamWidget::buildLayout(QVBoxLayout* layout)
{
    mFollowCameraComboBox->addItem("None");

    QComboBox* frameComboBox = new QComboBox(this);
    frameComboBox->addItem("Local");
    frameComboBox->addItem("Global");

    QComboBox* imageryComboBox = new QComboBox(this);
    imageryComboBox->addItem("None");
    imageryComboBox->addItem("Map (Google)");
    imageryComboBox->addItem("Satellite (Google)");

    QCheckBox* worldGridCheckBox = new QCheckBox(this);
    worldGridCheckBox->setChecked(mGlobalViewParams->displayWorldGrid());

    QMapIterator<int, SystemViewParamsPtr> it(mSystemViewParamMap);
    while (it.hasNext())
    {
        QString text("MAV ");
        text += QString::number(it.key());
        mFollowCameraComboBox->addItem(text);

        addTab(it.key());
    }

    QFormLayout* formLayout = new QFormLayout;
    formLayout->addRow(tr("Follow Camera"), mFollowCameraComboBox);
    formLayout->addRow(tr("Frame"), frameComboBox);
    formLayout->addRow(tr("Imagery"), imageryComboBox);
    formLayout->addRow(tr("World Grid"), worldGridCheckBox);

    layout->addLayout(formLayout);
    layout->addWidget(mTabWidget);

    // connect signals/slots
    connect(mFollowCameraComboBox, SIGNAL(currentIndexChanged(const QString&)),
            mGlobalViewParams.data(), SLOT(followCameraChanged(const QString&)));
    connect(frameComboBox, SIGNAL(currentIndexChanged(const QString&)),
            mGlobalViewParams.data(), SLOT(frameChanged(const QString&)));
    connect(imageryComboBox, SIGNAL(currentIndexChanged(int)),
            mGlobalViewParams.data(), SLOT(imageryTypeChanged(int)));
    connect(worldGridCheckBox, SIGNAL(stateChanged(int)),
            mGlobalViewParams.data(), SLOT(toggleWorldGrid(int)));
}

void
ViewParamWidget::addTab(int systemId)
{
    // add widgets that configure system-specific parameters
    SystemViewParamsPtr systemViewParams = mSystemViewParamMap[systemId];

    QWidget* page = new QWidget;

    QCheckBox* colorPointCloudCheckBox = new QCheckBox(this);
    colorPointCloudCheckBox->setChecked(systemViewParams->colorPointCloudByDistance());

    QCheckBox* localGridCheckBox = new QCheckBox(this);
    localGridCheckBox->setChecked(systemViewParams->displayLocalGrid());

    QComboBox* modelComboBox = new QComboBox(this);
    for (int i = 0; i < systemViewParams->modelNames().size(); ++i)
    {
        modelComboBox->addItem(systemViewParams->modelNames().at(i));
    }

    QCheckBox* obstacleListCheckBox = new QCheckBox(this);
    obstacleListCheckBox->setChecked(systemViewParams->displayObstacleList());

    QCheckBox* plannedPathCheckBox = new QCheckBox(this);
    plannedPathCheckBox->setChecked(systemViewParams->displayPlannedPath());

    QCheckBox* pointCloudCheckBox = new QCheckBox(this);
    pointCloudCheckBox->setChecked(systemViewParams->displayPointCloud());

    QCheckBox* rgbdCheckBox = new QCheckBox(this);
    rgbdCheckBox->setChecked(systemViewParams->displayRGBD());

    QCheckBox* setpointsCheckBox = new QCheckBox(this);
    setpointsCheckBox->setChecked(systemViewParams->displaySetpoints());

    mSetpointHistoryLengthSpinBox = new QSpinBox(this);
    mSetpointHistoryLengthSpinBox->setRange(1, 10000);
    mSetpointHistoryLengthSpinBox->setSingleStep(10);
    mSetpointHistoryLengthSpinBox->setValue(systemViewParams->setpointHistoryLength());
    mSetpointHistoryLengthSpinBox->setEnabled(systemViewParams->displaySetpoints());

    QCheckBox* targetCheckBox = new QCheckBox(this);
    targetCheckBox->setChecked(systemViewParams->displayTarget());

    QCheckBox* trailsCheckBox = new QCheckBox(this);
    trailsCheckBox->setChecked(systemViewParams->displayTrails());

    QCheckBox* waypointsCheckBox = new QCheckBox(this);
    waypointsCheckBox->setChecked(systemViewParams->displayWaypoints());

    QFormLayout* formLayout = new QFormLayout;
    page->setLayout(formLayout);

    formLayout->addRow(tr("Color Point Cloud"), colorPointCloudCheckBox);
    formLayout->addRow(tr("Local Grid"), localGridCheckBox);
    formLayout->addRow(tr("Model"), modelComboBox);
    formLayout->addRow(tr("Obstacles"), obstacleListCheckBox);
    formLayout->addRow(tr("Planned Path"), plannedPathCheckBox);
    formLayout->addRow(tr("Point Cloud"), pointCloudCheckBox);
    formLayout->addRow(tr("RGBD"), rgbdCheckBox);
    formLayout->addRow(tr("Setpoints"), setpointsCheckBox);
    formLayout->addRow(tr("Setpoint History Length"), mSetpointHistoryLengthSpinBox);
    formLayout->addRow(tr("Target"), targetCheckBox);
    formLayout->addRow(tr("Trails"), trailsCheckBox);
    formLayout->addRow(tr("Waypoints"), waypointsCheckBox);

    QString label("MAV ");
    label += QString::number(systemId);

    mTabWidget->addTab(page, label);

    // connect signals / slots
    connect(colorPointCloudCheckBox, SIGNAL(stateChanged(int)),
            systemViewParams.data(), SLOT(toggleColorPointCloud(int)));
    connect(localGridCheckBox, SIGNAL(stateChanged(int)),
            systemViewParams.data(), SLOT(toggleLocalGrid(int)));
    connect(modelComboBox, SIGNAL(currentIndexChanged(int)),
            systemViewParams.data(), SLOT(modelChanged(int)));
    connect(obstacleListCheckBox, SIGNAL(stateChanged(int)),
            systemViewParams.data(), SLOT(toggleObstacleList(int)));
    connect(plannedPathCheckBox, SIGNAL(stateChanged(int)),
            systemViewParams.data(), SLOT(togglePlannedPath(int)));
    connect(pointCloudCheckBox, SIGNAL(stateChanged(int)),
            systemViewParams.data(), SLOT(togglePointCloud(int)));
    connect(rgbdCheckBox, SIGNAL(stateChanged(int)),
            systemViewParams.data(), SLOT(toggleRGBD(int)));
    connect(setpointsCheckBox, SIGNAL(stateChanged(int)),
            systemViewParams.data(), SLOT(toggleSetpoints(int)));
    connect(setpointsCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(setpointsCheckBoxToggled(int)));
    connect(mSetpointHistoryLengthSpinBox, SIGNAL(valueChanged(int)),
            systemViewParams.data(), SLOT(setSetpointHistoryLength(int)));
    connect(targetCheckBox, SIGNAL(stateChanged(int)),
            systemViewParams.data(), SLOT(toggleTarget(int)));
    connect(trailsCheckBox, SIGNAL(stateChanged(int)),
            systemViewParams.data(), SLOT(toggleTrails(int)));
    connect(waypointsCheckBox, SIGNAL(stateChanged(int)),
            systemViewParams.data(), SLOT(toggleWaypoints(int)));
}
