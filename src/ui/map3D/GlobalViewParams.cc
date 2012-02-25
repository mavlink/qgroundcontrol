#include "GlobalViewParams.h"

#include <QStringList>

GlobalViewParams::GlobalViewParams()
 : mDisplayTerrain(true)
 , mDisplayWorldGrid(true)
 , mImageryType(Imagery::BLANK_MAP)
 , mFollowCameraId(-1)
 , mFrame(MAV_FRAME_LOCAL_NED)
{

}

bool&
GlobalViewParams::displayTerrain(void)
{
    return mDisplayTerrain;
}

bool
GlobalViewParams::displayTerrain(void) const
{
    return mDisplayTerrain;
}

bool&
GlobalViewParams::displayWorldGrid(void)
{
    return mDisplayWorldGrid;
}

bool
GlobalViewParams::displayWorldGrid(void) const
{
    return mDisplayWorldGrid;
}

QVector3D&
GlobalViewParams::imageryOffset(void)
{
    return mImageryOffset;
}

QVector3D
GlobalViewParams::imageryOffset(void) const
{
    return mImageryOffset;
}

QString&
GlobalViewParams::imageryPath(void)
{
    return mImageryPath;
}

QString
GlobalViewParams::imageryPath(void) const
{
    return mImageryPath;
}

Imagery::Type&
GlobalViewParams::imageryType(void)
{
    return mImageryType;
}

Imagery::Type
GlobalViewParams::imageryType(void) const
{
    return mImageryType;
}

int&
GlobalViewParams::followCameraId(void)
{
    return mFollowCameraId;
}

int
GlobalViewParams::followCameraId(void) const
{
    return mFollowCameraId;
}

MAV_FRAME&
GlobalViewParams::frame(void)
{
    return mFrame;
}

MAV_FRAME
GlobalViewParams::frame(void) const
{
    return mFrame;
}

void
GlobalViewParams::signalImageryParamsChanged(void)
{
    emit imageryParamsChanged();
}

QVector3D&
GlobalViewParams::terrainPositionOffset(void)
{
    return mTerrainPositionOffset;
}

QVector3D
GlobalViewParams::terrainPositionOffset(void) const
{
    return mTerrainPositionOffset;
}

QVector3D&
GlobalViewParams::terrainAttitudeOffset(void)
{
    return mTerrainAttitudeOffset;
}

QVector3D
GlobalViewParams::terrainAttitudeOffset(void) const
{
    return mTerrainAttitudeOffset;
}

void
GlobalViewParams::followCameraChanged(const QString& text)
{
    int followCameraId = -1;

    if (text.compare("None") == 0)
    {
        followCameraId = -1;
    }
    else
    {
        QStringList list = text.split(" ", QString::SkipEmptyParts);

        followCameraId = list.back().toInt();
    }

    if (followCameraId != mFollowCameraId)
    {
        mFollowCameraId = followCameraId;
        emit followCameraChanged(mFollowCameraId);
    }
}

void
GlobalViewParams::frameChanged(const QString& text)
{
    if (text.compare("Global") == 0)
    {
       mFrame = MAV_FRAME_GLOBAL;
    }
    else if (text.compare("Local") == 0)
    {
       mFrame = MAV_FRAME_LOCAL_NED;
    }
}

void
GlobalViewParams::toggleWorldGrid(int state)
{
    if (state == Qt::Checked)
    {
        mDisplayWorldGrid = true;
    }
    else
    {
        mDisplayWorldGrid = false;
    }
}

void
GlobalViewParams::toggleTerrain(int state)
{
    if (state == Qt::Checked)
    {
        mDisplayTerrain = true;
    }
    else
    {
        mDisplayTerrain = false;
    }
}
