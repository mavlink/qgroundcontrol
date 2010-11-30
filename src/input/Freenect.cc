#include "Freenect.h"

#include <cmath>
#include <string.h>
#include <QDebug>

Freenect::Freenect()
    : context(NULL)
    , device(NULL)
    , tiltAngle(0)
{
    for (int i = 0; i < 2048; ++i)
    {
        float v = static_cast<float>(i) / 2048.0f;
        v = powf(v, 3.0f) * 6.0f;
        gammaTable[i] = static_cast<unsigned short>(v * 6.0f * 256.0f);
    }
}

Freenect::~Freenect()
{
    if (device != NULL)
    {
        freenect_stop_depth(device);
        freenect_stop_rgb(device);
    }

    freenect_close_device(device);

    freenect_shutdown(context);
}

bool
Freenect::init(int userDeviceNumber)
{
    if (freenect_init(&context, NULL) < 0)
    {
        return false;
    }

    freenect_set_log_level(context, FREENECT_LOG_DEBUG);

    if (freenect_num_devices(context) < 1)
    {
        return false;
    }

    if (freenect_open_device(context, &device, userDeviceNumber) < 0)
    {
        return false;
    }

    freenect_set_user(device, this);

    memset(rgb, 0, FREENECT_RGB_SIZE);
    memset(depth, 0, FREENECT_DEPTH_SIZE);

    // set Kinect parameters
    if (freenect_set_tilt_degs(device, tiltAngle) != 0)
    {
        return false;
    }
    if (freenect_set_led(device, LED_RED) != 0)
    {
        return false;
    }
    if (freenect_set_rgb_format(device, FREENECT_FORMAT_RGB) != 0)
    {
        return false;
    }
    if (freenect_set_depth_format(device, FREENECT_FORMAT_11_BIT) != 0)
    {
        return false;
    }
    freenect_set_rgb_callback(device, rgbCallback);
    freenect_set_depth_callback(device, depthCallback);

    if (freenect_start_rgb(device) != 0)
    {
        return false;
    }
    if (freenect_start_depth(device) != 0)
    {
        return false;
    }

    thread.reset(new FreenectThread(device));
    thread->start();

    return true;
}

bool
Freenect::process(void)
{
    if (freenect_process_events(context) < 0)
    {
        return false;
    }

    //libfreenect changed some access functions in one of the new revisions
    freenect_raw_device_state state;
    freenect_get_mks_accel(&state, &ax, &ay, &az);
    //tiltAngle = freenect_get_tilt_degs(&state);

    //these are the old access functions
    //freenect_get_raw_accel(device, &ax, &ay, &az);
    //freenect_get_mks_accel(device, &dx, &dy, &dz);

    return true;
}

QSharedPointer<QByteArray>
Freenect::getRgbData(void)
{
    QMutexLocker locker(&rgbMutex);
    return QSharedPointer<QByteArray>(
            new QByteArray(rgb, FREENECT_RGB_SIZE));
}

QSharedPointer<QByteArray>
Freenect::getRawDepthData(void)
{
    QMutexLocker locker(&depthMutex);
    return QSharedPointer<QByteArray>(
            new QByteArray(depth, FREENECT_DEPTH_SIZE));
}

QSharedPointer<QByteArray>
Freenect::getDistanceData(void)
{
    QMutexLocker locker(&distanceMutex);
    return QSharedPointer<QByteArray>(
            new QByteArray(reinterpret_cast<char *>(distance),
                           FREENECT_FRAME_PIX * sizeof(float)));
}

QSharedPointer<QByteArray>
Freenect::getColoredDepthData(void)
{
    QMutexLocker locker(&coloredDepthMutex);
    return QSharedPointer<QByteArray>(
            new QByteArray(coloredDepth, FREENECT_RGB_SIZE));
}

int
Freenect::getTiltAngle(void) const
{
    return tiltAngle;
}

void
Freenect::setTiltAngle(int angle)
{
    if (angle > 30)
    {
        angle = 30;
    }
    if (angle < -30)
    {
        angle = -30;
    }

    tiltAngle = angle;
}

Freenect::FreenectThread::FreenectThread(freenect_device* _device)
{
    device = _device;
}

void
Freenect::FreenectThread::run(void)
{
    Freenect* freenect = static_cast<Freenect *>(freenect_get_user(device));
    while (1)
    {
        freenect->process();
    }
}

void
Freenect::rgbCallback(freenect_device* device, freenect_pixel* rgb, uint32_t timestamp)
{
    Freenect* freenect = static_cast<Freenect *>(freenect_get_user(device));

    QMutexLocker locker(&freenect->rgbMutex);
    memcpy(freenect->rgb, rgb, FREENECT_RGB_SIZE);
}

void
Freenect::depthCallback(freenect_device* device, void* depth, uint32_t timestamp)
{
    Freenect* freenect = static_cast<Freenect *>(freenect_get_user(device));
    freenect_depth* data = reinterpret_cast<freenect_depth *>(depth);

    QMutexLocker depthLocker(&freenect->depthMutex);
    memcpy(freenect->depth, data, FREENECT_DEPTH_SIZE);

    QMutexLocker distanceLocker(&freenect->distanceMutex);
    for (int i = 0; i < FREENECT_FRAME_PIX; ++i)
    {
        freenect->distance[i] =
                100.f / (-0.00307f * static_cast<float>(data[i]) + 3.33f);
    }

    QMutexLocker coloredDepthLocker(&freenect->coloredDepthMutex);
    unsigned short* src = reinterpret_cast<unsigned short *>(data);
    unsigned char* dst = reinterpret_cast<unsigned char *>(freenect->coloredDepth);
    for (int i = 0; i < FREENECT_FRAME_PIX; ++i)
    {
        unsigned short pval = freenect->gammaTable[src[i]];
        unsigned short lb = pval & 0xFF;
        switch (pval >> 8)
        {
        case 0:
            dst[3 * i] = 255;
            dst[3 * i + 1] = 255 - lb;
            dst[3 * i + 2] = 255 - lb;
            break;
        case 1:
            dst[3 * i] = 255;
            dst[3 * i + 1] = lb;
            dst[3 * i + 2] = 0;
            break;
        case 2:
            dst[3 * i] = 255 - lb;
            dst[3 * i + 1] = 255;
            dst[3 * i + 2] = 0;
            break;
        case 3:
            dst[3 * i] = 0;
            dst[3 * i + 1] = 255;
            dst[3 * i + 2] = lb;
            break;
        case 4:
            dst[3 * i] = 0;
            dst[3 * i + 1] = 255 - lb;
            dst[3 * i + 2] = 255;
            break;
        case 5:
            dst[3 * i] = 0;
            dst[3 * i + 1] = 0;
            dst[3 * i + 2] = 255 - lb;
            break;
        default:
            dst[3 * i] = 0;
            dst[3 * i + 1] = 0;
            dst[3 * i + 2] = 0;
            break;
        }
    }
}
