#include "GlobalViewParams.h"

#include <QStringList>

GlobalViewParams::GlobalViewParams()
 : mDisplayWorldGrid(true)
 , mImageryType(Imagery::BLANK_MAP)
 , mFollowCameraId(-1)
 , mFrame(MAV_FRAME_LOCAL_NED)
{

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
GlobalViewParams::imageryTypeChanged(int index)
{
    mImageryType = static_cast<Imagery::Type>(index);
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
