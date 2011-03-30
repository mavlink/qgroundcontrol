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

#ifndef FREENECT_H
#define FREENECT_H

#include <libfreenect/libfreenect.h>
#include <QMatrix4x4>
#include <QMutex>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QThread>
#include <QVector>
#include <QVector2D>
#include <QVector3D>

class Freenect
{
public:
    Freenect();
    ~Freenect();

    bool init(int userDeviceNumber = 0);
    bool process(void);

    QSharedPointer<QByteArray> getRgbData(void);
    QSharedPointer<QByteArray> getRawDepthData(void);
    QSharedPointer<QByteArray> getColoredDepthData(void);
    QSharedPointer< QVector<QVector3D> > get3DPointCloudData(void);

    typedef struct {
        double x;
        double y;
        double z;
        unsigned char r;
        unsigned char g;
        unsigned char b;
    } Vector6D;
    QSharedPointer< QVector<Vector6D> > get6DPointCloudData();

    int getTiltAngle(void) const;
    void setTiltAngle(int angle);

private:
    typedef struct {
        // coordinates of principal point
        double cx;
        double cy;

        // focal length in pixels
        double fx;
        double fy;

        // distortion parameters
        double k[5];

    } IntrinsicCameraParameters;

    void readConfigFile(void);

    void rectifyPoint(const QVector2D& originalPoint,
                      QVector2D& rectifiedPoint,
                      const IntrinsicCameraParameters& params);
    void unrectifyPoint(const QVector2D& rectifiedPoint,
                        QVector2D& originalPoint,
                        const IntrinsicCameraParameters& params);
    void projectPixelTo3DRay(const QVector2D& pixel, QVector3D& ray,
                             const IntrinsicCameraParameters& params);

    static void videoCallback(freenect_device* device, void* video, uint32_t timestamp);
    static void depthCallback(freenect_device* device, void* depth, uint32_t timestamp);

    freenect_context* context;
    freenect_device* device;

    class FreenectThread : public QThread
    {
    public:
        explicit FreenectThread(freenect_device* _device);

    protected:
        virtual void run(void);

        freenect_device* device;
    };
    QScopedPointer<FreenectThread> thread;

    IntrinsicCameraParameters rgbCameraParameters;
    IntrinsicCameraParameters depthCameraParameters;

    QMatrix4x4 transformMatrix;
    double baseline;
    double disparityOffset;

    // tilt angle of Kinect camera
    int tiltAngle;

    // rgbd data
    char rgb[FREENECT_VIDEO_RGB_SIZE];
    QMutex rgbMutex;

    char depth[FREENECT_DEPTH_11BIT_SIZE];
    QMutex depthMutex;

    char coloredDepth[FREENECT_VIDEO_RGB_SIZE];
    QMutex coloredDepthMutex;

    // accelerometer data
    double ax, ay, az;
    double dx, dy, dz;

    // gamma map
    unsigned short gammaTable[2048];

    QVector3D depthProjectionMatrix[FREENECT_FRAME_PIX];
    QVector2D rgbRectificationMap[FREENECT_FRAME_PIX];

    // variables for use outside class
    QSharedPointer<QByteArray> rgbData;
    QSharedPointer<QByteArray> rawDepthData;
    QSharedPointer<QByteArray> coloredDepthData;
    QSharedPointer< QVector<QVector3D> > pointCloud3D;
    QSharedPointer< QVector<Vector6D> > pointCloud6D;
};

#endif // FREENECT_H
