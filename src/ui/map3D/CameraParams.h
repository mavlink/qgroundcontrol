#ifndef CAMERAPARAMS_H
#define CAMERAPARAMS_H

class CameraParams
{
public:
    CameraParams();

    float& minZoomRange(void);
    float minZoomRange(void) const;

    float& fov(void);
    float fov(void) const;

    float& minClipRange(void);
    float minClipRange(void) const;

    float& maxClipRange(void);
    float maxClipRange(void) const;

private:
    float mMinZoomRange;
    float mFov;
    float mMinClipRange;
    float mMaxClipRange;
};

#endif // CAMERAPARAMS_H
