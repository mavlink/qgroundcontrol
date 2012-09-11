/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Definition of the class Freenect.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#include "Freenect.h"

#include <cmath>
#include <string.h>
#include <QSettings>

Freenect::Freenect()
    : context(NULL)
    , device(NULL)
    , tiltAngle(0)
    , rgbData(new QByteArray)
    , rawDepthData(new QByteArray)
    , coloredDepthData(new QByteArray)
    , pointCloud3D(new QVector<QVector3D>)
    , pointCloud6D(new QVector<Vector6D>)
{

}

Freenect::~Freenect()
{
    if (device != NULL) {
        freenect_stop_depth(device);
        freenect_stop_video(device);
    }

    freenect_close_device(device);

    freenect_shutdown(context);
}

bool
Freenect::init(int userDeviceNumber)
{
    // read in settings
    readConfigFile();

    // populate gamma lookup table
    for (int i = 0; i < 2048; ++i) {
        float v = static_cast<float>(i) / 2048.0f;
        v = powf(v, 3.0f) * 6.0f;
        gammaTable[i] = static_cast<unsigned short>(v * 6.0f * 256.0f);
    }

    // populate depth projection matrix
    for (int i = 0; i < FREENECT_FRAME_H; ++i) {
        for (int j = 0; j < FREENECT_FRAME_W; ++j) {
            QVector2D originalPoint(j, i);
            QVector2D rectifiedPoint;
            rectifyPoint(originalPoint, rectifiedPoint, depthCameraParameters);

            QVector3D rectifiedRay;
            projectPixelTo3DRay(rectifiedPoint, rectifiedRay, depthCameraParameters);

            depthProjectionMatrix[i * FREENECT_FRAME_W + j] = rectifiedRay;

            rectifyPoint(originalPoint, rectifiedPoint, rgbCameraParameters);
            rgbRectificationMap[i * FREENECT_FRAME_W + j] = rectifiedPoint;
        }
    }

    if (freenect_init(&context, NULL) < 0) {
        return false;
    }

    freenect_set_log_level(context, FREENECT_LOG_DEBUG);

    if (freenect_num_devices(context) < 1) {
        return false;
    }

    if (freenect_open_device(context, &device, userDeviceNumber) < 0) {
        return false;
    }

    freenect_set_user(device, this);

    memset(rgb, 0, FREENECT_VIDEO_RGB_SIZE);
    memset(depth, 0, FREENECT_DEPTH_11BIT_SIZE);

    // set Kinect parameters
    if (freenect_set_tilt_degs(device, tiltAngle) != 0) {
        return false;
    }
    if (freenect_set_led(device, LED_RED) != 0) {
        return false;
    }
    if (freenect_set_video_format(device, FREENECT_VIDEO_RGB) != 0) {
        return false;
    }
    if (freenect_set_depth_format(device, FREENECT_DEPTH_11BIT) != 0) {
        return false;
    }
    freenect_set_video_callback(device, videoCallback);
    freenect_set_depth_callback(device, depthCallback);

    if (freenect_start_depth(device) != 0) {
        return false;
    }
    if (freenect_start_video(device) != 0) {
        return false;
    }

    thread.reset(new FreenectThread(device));
    thread->start();

    return true;
}

bool
Freenect::process(void)
{
    if (freenect_process_events(context) < 0) {
        return false;
    }

    freenect_raw_tilt_state* state;
    freenect_update_tilt_state(device);
    state = freenect_get_tilt_state(device);
    freenect_get_mks_accel(state, &ax, &ay, &az);

    return true;
}

QSharedPointer<QByteArray>
Freenect::getRgbData(void)
{
    QMutexLocker locker(&rgbMutex);
    rgbData->clear();
    rgbData->append(rgb, FREENECT_VIDEO_RGB_SIZE);

    return rgbData;
}

QSharedPointer<QByteArray>
Freenect::getRawDepthData(void)
{
    QMutexLocker locker(&depthMutex);
    rawDepthData->clear();
    rawDepthData->append(depth, FREENECT_DEPTH_11BIT_SIZE);

    return rawDepthData;
}

QSharedPointer<QByteArray>
Freenect::getColoredDepthData(void)
{
    QMutexLocker locker(&coloredDepthMutex);
    coloredDepthData->clear();
    coloredDepthData->append(coloredDepth, FREENECT_VIDEO_RGB_SIZE);

    return coloredDepthData;
}

QSharedPointer< QVector<QVector3D> >
Freenect::get3DPointCloudData(void)
{
    QMutexLocker locker(&depthMutex);

    pointCloud3D->clear();
    unsigned short* data = reinterpret_cast<unsigned short*>(depth);
    for (int i = 0; i < FREENECT_FRAME_PIX; ++i) {
        if (data[i] > 0 && data[i] <= 2048) {
            double range = baseline * depthCameraParameters.fx
                           / (1.0 / 8.0 * (disparityOffset
                                           - static_cast<double>(data[i])));

            if (range > 0.0f) {
                QVector3D ray = depthProjectionMatrix[i];
                ray *= range;

                pointCloud3D->push_back(QVector3D(ray.x(), ray.y(), ray.z()));
            }
        }
    }

    return pointCloud3D;
}

QSharedPointer< QVector<Freenect::Vector6D> >
Freenect::get6DPointCloudData(void)
{
    get3DPointCloudData();

    pointCloud6D->clear();
    for (int i = 0; i < pointCloud3D->size(); ++i) {
        Vector6D point;

        point.x = pointCloud3D->at(i).x();
        point.y = pointCloud3D->at(i).y();
        point.z = pointCloud3D->at(i).z();

        QVector4D transformedPoint = transformMatrix * QVector4D(point.x, point.y, point.z, 1.0);

        float iz = 1.0 / transformedPoint.z();
        QVector2D rectifiedPoint(transformedPoint.x() * iz * rgbCameraParameters.fx + rgbCameraParameters.cx,
                                 transformedPoint.y() * iz * rgbCameraParameters.fy + rgbCameraParameters.cy);

        QVector2D originalPoint;
        unrectifyPoint(rectifiedPoint, originalPoint, rgbCameraParameters);

        if (originalPoint.x() >= 0.0 && originalPoint.x() < FREENECT_FRAME_W &&
                originalPoint.y() >= 0.0 && originalPoint.y() < FREENECT_FRAME_H) {
            int x = static_cast<int>(originalPoint.x());
            int y = static_cast<int>(originalPoint.y());
            unsigned char* pixel = reinterpret_cast<unsigned char*>(rgb)
                                   + (y * FREENECT_FRAME_W + x) * 3;

            point.r = pixel[0];
            point.g = pixel[1];
            point.b = pixel[2];

            pointCloud6D->push_back(point);
        }
    }

    return pointCloud6D;
}

int
Freenect::getTiltAngle(void) const
{
    return tiltAngle;
}

void
Freenect::setTiltAngle(int angle)
{
    if (angle > 30) {
        angle = 30;
    }
    if (angle < -30) {
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
    while (1) {
        freenect->process();
    }
}

void
Freenect::readConfigFile(void)
{
    QSettings settings("src/data/kinect.cal", QSettings::IniFormat, 0);

    rgbCameraParameters.cx = settings.value("rgb/principal_point/x").toDouble();
    rgbCameraParameters.cy = settings.value("rgb/principal_point/y").toDouble();
    rgbCameraParameters.fx = settings.value("rgb/focal_length/x").toDouble();
    rgbCameraParameters.fy = settings.value("rgb/focal_length/y").toDouble();
    rgbCameraParameters.k[0] = settings.value("rgb/distortion/k1").toDouble();
    rgbCameraParameters.k[1] = settings.value("rgb/distortion/k2").toDouble();
    rgbCameraParameters.k[2] = settings.value("rgb/distortion/k3").toDouble();
    rgbCameraParameters.k[3] = settings.value("rgb/distortion/k4").toDouble();
    rgbCameraParameters.k[4] = settings.value("rgb/distortion/k5").toDouble();

    depthCameraParameters.cx = settings.value("depth/principal_point/x").toDouble();
    depthCameraParameters.cy = settings.value("depth/principal_point/y").toDouble();
    depthCameraParameters.fx = settings.value("depth/focal_length/x").toDouble();
    depthCameraParameters.fy = settings.value("depth/focal_length/y").toDouble();
    depthCameraParameters.k[0] = settings.value("depth/distortion/k1").toDouble();
    depthCameraParameters.k[1] = settings.value("depth/distortion/k2").toDouble();
    depthCameraParameters.k[2] = settings.value("depth/distortion/k3").toDouble();
    depthCameraParameters.k[3] = settings.value("depth/distortion/k4").toDouble();
    depthCameraParameters.k[4] = settings.value("depth/distortion/k5").toDouble();

    transformMatrix = QMatrix4x4(settings.value("transform/R11").toDouble(),
                                 settings.value("transform/R12").toDouble(),
                                 settings.value("transform/R13").toDouble(),
                                 settings.value("transform/Tx").toDouble(),
                                 settings.value("transform/R21").toDouble(),
                                 settings.value("transform/R22").toDouble(),
                                 settings.value("transform/R23").toDouble(),
                                 settings.value("transform/Ty").toDouble(),
                                 settings.value("transform/R31").toDouble(),
                                 settings.value("transform/R32").toDouble(),
                                 settings.value("transform/R33").toDouble(),
                                 settings.value("transform/Tz").toDouble(),
                                 0.0, 0.0, 0.0, 1.0);
    transformMatrix = transformMatrix.inverted();

    baseline = settings.value("transform/baseline").toDouble();
    disparityOffset = settings.value("transform/disparity_offset").toDouble();
}

void
Freenect::rectifyPoint(const QVector2D& originalPoint,
                       QVector2D& rectifiedPoint,
                       const IntrinsicCameraParameters& params)
{
    double x = (originalPoint.x() - params.cx) / params.fx;
    double y = (originalPoint.y() - params.cy) / params.fy;

    double x0 = x;
    double y0 = y;

    // eliminate lens distortion iteratively
    for (int i = 0; i < 4; ++i) {
        double r2 = x * x + y * y;

        // tangential distortion vector [dx dy]
        double dx = 2 * params.k[2] * x * y + params.k[3] * (r2 + 2 * x * x);
        double dy = params.k[2] * (r2 + 2 * y * y) + 2 * params.k[3] * x * y;

        double icdist = 1.0 / (1.0 + r2 * (params.k[0] + r2 * (params.k[1] + r2 * params.k[4])));
        x = (x0 - dx) * icdist;
        y = (y0 - dy) * icdist;
    }

    rectifiedPoint.setX(x * params.fx + params.cx);
    rectifiedPoint.setY(y * params.fy + params.cy);
}

void
Freenect::unrectifyPoint(const QVector2D& rectifiedPoint,
                         QVector2D& originalPoint,
                         const IntrinsicCameraParameters& params)
{
    double x = (rectifiedPoint.x() - params.cx) / params.fx;
    double y = (rectifiedPoint.y() - params.cy) / params.fy;

    double r2 = x * x + y * y;

    // tangential distortion vector [dx dy]
    double dx = 2 * params.k[2] * x * y + params.k[3] * (r2 + 2 * x * x);
    double dy = params.k[2] * (r2 + 2 * y * y) + 2 * params.k[3] * x * y;

    double cdist = 1.0 + r2 * (params.k[0] + r2 * (params.k[1] + r2 * params.k[4]));
    x = x * cdist + dx;
    y = y * cdist + dy;

    originalPoint.setX(x * params.fx + params.cx);
    originalPoint.setY(y * params.fy + params.cy);
}

void
Freenect::projectPixelTo3DRay(const QVector2D& pixel, QVector3D& ray,
                              const IntrinsicCameraParameters& params)
{
    ray.setX((pixel.x() - params.cx) / params.fx);
    ray.setY((pixel.y() - params.cy) / params.fy);
    ray.setZ(1.0);
}

void
Freenect::videoCallback(freenect_device* device, void* video, uint32_t timestamp)
{
    Freenect* freenect = static_cast<Freenect *>(freenect_get_user(device));

    QMutexLocker locker(&freenect->rgbMutex);
    memcpy(freenect->rgb, video, FREENECT_VIDEO_RGB_SIZE);
}

void
Freenect::depthCallback(freenect_device* device, void* depth, uint32_t timestamp)
{
    Freenect* freenect = static_cast<Freenect *>(freenect_get_user(device));
    uint16_t* data = reinterpret_cast<uint16_t *>(depth);

    QMutexLocker depthLocker(&freenect->depthMutex);
    memcpy(freenect->depth, data, FREENECT_DEPTH_11BIT_SIZE);

    QMutexLocker coloredDepthLocker(&freenect->coloredDepthMutex);
    unsigned short* src = reinterpret_cast<unsigned short *>(data);
    unsigned char* dst = reinterpret_cast<unsigned char *>(freenect->coloredDepth);
    for (int i = 0; i < FREENECT_FRAME_PIX; ++i) {
        unsigned short pval = freenect->gammaTable[src[i]];
        unsigned short lb = pval & 0xFF;
        switch (pval >> 8) {
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
