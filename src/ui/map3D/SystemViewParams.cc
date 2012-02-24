#include "SystemViewParams.h"

SystemViewParams::SystemViewParams(int systemId)
 : mSystemId(systemId)
 , mColorPointCloudByDistance(false)
 , mDisplayLocalGrid(false)
 , mDisplayObstacleList(true)
 , mDisplayPlannedPath(true)
 , mDisplayPointCloud(true)
 , mDisplayRGBD(false)
 , mDisplaySetpoints(true)
 , mDisplayTarget(true)
 , mDisplayTrails(true)
 , mDisplayWaypoints(true)
 , mModelIndex(-1)
 , mSetpointHistoryLength(100)
{

}

bool&
SystemViewParams::colorPointCloudByDistance(void)
{
    return mColorPointCloudByDistance;
}

bool
SystemViewParams::colorPointCloudByDistance(void) const
{
    return mColorPointCloudByDistance;
}

bool&
SystemViewParams::displayLocalGrid(void)
{
    return mDisplayLocalGrid;
}

bool
SystemViewParams::displayLocalGrid(void) const
{
    return mDisplayLocalGrid;
}

bool&
SystemViewParams::displayObstacleList(void)
{
    return mDisplayObstacleList;
}

bool
SystemViewParams::displayObstacleList(void) const
{
    return mDisplayObstacleList;
}

QMap<QString,bool>&
SystemViewParams::displayOverlay(void)
{
    return mDisplayOverlay;
}

QMap<QString,bool>
SystemViewParams::displayOverlay(void) const
{
    return mDisplayOverlay;
}

bool&
SystemViewParams::displayPlannedPath(void)
{
    return mDisplayPlannedPath;
}

bool
SystemViewParams::displayPlannedPath(void) const
{
    return mDisplayPlannedPath;
}

bool&
SystemViewParams::displayPointCloud(void)
{
    return mDisplayPointCloud;
}

bool
SystemViewParams::displayPointCloud(void) const
{
    return mDisplayPointCloud;
}

bool&
SystemViewParams::displayRGBD(void)
{
    return mDisplayRGBD;
}

bool
SystemViewParams::displayRGBD(void) const
{
    return mDisplayRGBD;
}

bool&
SystemViewParams::displaySetpoints(void)
{
    return mDisplaySetpoints;
}

bool
SystemViewParams::displaySetpoints(void) const
{
    return mDisplaySetpoints;
}

bool&
SystemViewParams::displayTarget(void)
{
    return mDisplayTarget;
}

bool
SystemViewParams::displayTarget(void) const
{
    return mDisplayTarget;
}

bool&
SystemViewParams::displayTrails(void)
{
    return mDisplayTrails;
}

bool
SystemViewParams::displayTrails(void) const
{
    return mDisplayTrails;
}

bool&
SystemViewParams::displayWaypoints(void)
{
    return mDisplayWaypoints;
}

bool
SystemViewParams::displayWaypoints(void) const
{
    return mDisplayWaypoints;
}

int&
SystemViewParams::modelIndex(void)
{
    return mModelIndex;
}

int
SystemViewParams::modelIndex(void) const
{
    return mModelIndex;
}

QVector<QString>&
SystemViewParams::modelNames(void)
{
    return mModelNames;
}

const QVector<QString>&
SystemViewParams::modelNames(void) const
{
    return mModelNames;
}

int&
SystemViewParams::setpointHistoryLength(void)
{
    return mSetpointHistoryLength;
}

int
SystemViewParams::setpointHistoryLength(void) const
{
    return mSetpointHistoryLength;
}

void
SystemViewParams::modelChanged(int index)
{
    mModelIndex = index;

    emit modelChangedSignal(mSystemId, index);
}

void
SystemViewParams::setSetpointHistoryLength(int length)
{
    mSetpointHistoryLength = length;
}

void
SystemViewParams::toggleColorPointCloud(int state)
{
    if (state == Qt::Checked)
    {
        mColorPointCloudByDistance = true;
    }
    else
    {
        mColorPointCloudByDistance = false;
    }
}

void
SystemViewParams::toggleLocalGrid(int state)
{
    if (state == Qt::Checked)
    {
        mDisplayLocalGrid = true;
    }
    else
    {
        mDisplayLocalGrid = false;
    }
}

void
SystemViewParams::toggleObstacleList(int state)
{
    if (state == Qt::Checked)
    {
        mDisplayObstacleList = true;
    }
    else
    {
        mDisplayObstacleList = false;
    }
}

void
SystemViewParams::toggleOverlay(const QString& name)
{
    if (!mDisplayOverlay.contains(name))
    {
        return;
    }

    mDisplayOverlay[name] = !mDisplayOverlay[name];
}

void
SystemViewParams::togglePlannedPath(int state)
{
    if (state == Qt::Checked)
    {
        mDisplayPlannedPath = true;
    }
    else
    {
        mDisplayPlannedPath = false;
    }
}

void
SystemViewParams::togglePointCloud(int state)
{
    if (state == Qt::Checked)
    {
        mDisplayPointCloud = true;
    }
    else
    {
        mDisplayPointCloud = false;
    }
}

void
SystemViewParams::toggleRGBD(int state)
{
    if (state == Qt::Checked)
    {
        mDisplayRGBD = true;
    }
    else
    {
        mDisplayRGBD = false;
    }
}

void
SystemViewParams::toggleSetpoints(int state)
{
    if (state == Qt::Checked)
    {
        mDisplaySetpoints = true;
    }
    else
    {
        mDisplaySetpoints = false;
    }
}

void
SystemViewParams::toggleTarget(int state)
{
    if (state == Qt::Checked)
    {
        mDisplayTarget = true;
    }
    else
    {
        mDisplayTarget = false;
    }
}

void
SystemViewParams::toggleTrails(int state)
{
    if (state == Qt::Checked)
    {
        mDisplayTrails = true;
    }
    else
    {
        mDisplayTrails = false;
    }
}

void
SystemViewParams::toggleWaypoints(int state)
{
    if (state == Qt::Checked)
    {
        mDisplayWaypoints = true;
    }
    else
    {
        mDisplayWaypoints = false;
    }
}
