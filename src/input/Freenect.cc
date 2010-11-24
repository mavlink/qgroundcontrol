#include "Freenect.h"

#include <string.h>
#include <QDebug>

Freenect::Freenect()
    : context(NULL)
    , device(NULL)
    , tiltAngle(0)
{

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

    freenect_get_raw_accel(device, &ax, &ay, &az);
    freenect_get_mks_accel(device, &dx, &dy, &dz);

    return true;
}

QSharedPointer<QByteArray>
Freenect::getRgbData(void)
{
    QMutexLocker locker(&rgbMutex);
    return QSharedPointer<QByteArray>(new QByteArray(rgb, FREENECT_RGB_SIZE));
}

QSharedPointer<QByteArray>
Freenect::getDepthData(void)
{
    QMutexLocker locker(&depthMutex);
    return QSharedPointer<QByteArray>(new QByteArray(depth, FREENECT_DEPTH_SIZE));
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
Freenect::rgbCallback(freenect_device* device, freenect_pixel* rgb,
                      unsigned int timestamp)
{
    Freenect* freenect = static_cast<Freenect *>(freenect_get_user(device));

    QMutexLocker locker(&freenect->rgbMutex);
    memcpy(freenect->rgb, rgb, FREENECT_RGB_SIZE);
}

void
Freenect::depthCallback(freenect_device* device, void* depth,
                        unsigned int timestamp)
{
    Freenect* freenect = static_cast<Freenect *>(freenect_get_user(device));
    freenect_depth* data = reinterpret_cast<freenect_depth *>(depth);

    QMutexLocker locker(&freenect->depthMutex);
    memcpy(freenect->depth, data, FREENECT_DEPTH_SIZE);
}
