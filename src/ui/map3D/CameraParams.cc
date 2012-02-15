#include "CameraParams.h"

CameraParams::CameraParams()
 : mMinZoomRange(2.0f)
 , mFov(30.0f)
 , mMinClipRange(1.0f)
 , mMaxClipRange(10000.0f)
{

}

float&
CameraParams::minZoomRange(void)
{
    return mMinZoomRange;
}

float
CameraParams::minZoomRange(void) const
{
    return mMinZoomRange;
}

float&
CameraParams::fov(void)
{
    return mFov;
}

float
CameraParams::fov(void) const
{
    return mFov;
}

float&
CameraParams::minClipRange(void)
{
    return mMinClipRange;
}

float
CameraParams::minClipRange(void) const
{
    return mMinClipRange;
}

float&
CameraParams::maxClipRange(void)
{
    return mMaxClipRange;
}

float
CameraParams::maxClipRange(void) const
{
    return mMaxClipRange;
}
