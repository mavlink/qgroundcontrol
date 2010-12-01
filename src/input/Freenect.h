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
    QVector<QVector3D> get3DPointCloudData(void);

    typedef struct
    {
        double x;
        double y;
        double z;
        unsigned char r;
        unsigned char g;
        unsigned char b;
    } Vector6D;
    QVector<Vector6D> get6DPointCloudData(void);

    int getTiltAngle(void) const;
    void setTiltAngle(int angle);

private:
    typedef struct
    {
        // coordinates of principal point
        double cx;
        double cy;

        // focal length in pixels
        double fx;
        double fy;

        // distortion parameters
        double k[5];

    } IntrinsicCameraParameters;

    void rectifyPoint(const QVector2D& originalPoint,
                      QVector2D& rectifiedPoint,
                      const IntrinsicCameraParameters& params);
    void unrectifyPoint(const QVector2D& rectifiedPoint,
                        QVector2D& originalPoint,
                        const IntrinsicCameraParameters& params);
    void projectPixelTo3DRay(const QVector2D& pixel, QVector3D& ray,
                             const IntrinsicCameraParameters& params);

    static void rgbCallback(freenect_device* device, freenect_pixel* rgb, uint32_t timestamp);
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

    // tilt angle of Kinect camera
    int tiltAngle;

    // rgbd data
    char rgb[FREENECT_RGB_SIZE];
    QMutex rgbMutex;

    char depth[FREENECT_DEPTH_SIZE];
    QMutex depthMutex;

    char coloredDepth[FREENECT_RGB_SIZE];
    QMutex coloredDepthMutex;

    // accelerometer data
    double ax, ay, az;
    double dx, dy, dz;

    // gamma map
    unsigned short gammaTable[2048];

    QVector3D depthProjectionMatrix[FREENECT_FRAME_PIX];
    QVector2D rgbRectificationMap[FREENECT_FRAME_PIX];
};

#endif // FREENECT_H
