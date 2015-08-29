/**
 * @file
 *   @brief Definition of 3dConnexion 3dMouse interface for QGroundControl
 *
 *   @author Matthias Krebs <makrebs@student.ethz.ch>
 *
 */

#ifndef MOUSE6DOFINPUT_H
#define MOUSE6DOFINPUT_H

#include <QThread>

#ifdef QGC_MOUSE_ENABLED_WIN
#include "Mouse3DInput.h"
#endif //QGC_MOUSE_ENABLED_WIN

#include "UAS.h"
#include "Vehicle.h"

class Mouse6dofInput : public QThread
{
    Q_OBJECT

public:
#ifdef QGC_MOUSE_ENABLED_WIN
    Mouse6dofInput(Mouse3DInput* mouseInput);
#endif //QGC_MOUSE_ENABLED_WIN
#ifdef QGC_MOUSE_ENABLED_LINUX
    Mouse6dofInput(QWidget* parent);
#endif //QGC_MOUSE_ENABLED_LINUX

    ~Mouse6dofInput();
    void run();

    const double mouse3DMax;

protected:
    void init();

    UAS* uas;
    bool done;
    bool mouseActive;
    bool translationActive;
    bool rotationActive;

    double xValue;
    double yValue;
    double zValue;
    double aValue;
    double bValue;
    double cValue;


signals:
    /**
     * @brief Input of the 3d mouse has changed
     *
     * @param x Input x direction, range [-1, 1]
     * @param y Input y direction, range [-1, 1]
     * @param z Input z direction, range [-1, 1]
     * @param a Input x rotation,  range [-1, 1]
     * @param b Input y rotation,  range [-1, 1]
     * @param c Input z rotation,  range [-1, 1]
     */
    void mouse6dofChanged(double x, double y, double z, double a, double b, double c);

   /**
     * @brief Activity of translational 3DMouse inputs changed
     * @param translationEnable, true: translational inputs active; false: translational inputs ingored
     */
    void mouseTranslationActiveChanged(bool translationEnable);

   /**
     * @brief Activity of rotational 3DMouse inputs changed
     * @param rotationEnable, true: rotational inputs active; false: rotational inputs ingored
     */
    void mouseRotationActiveChanged(bool rotationEnable);

public slots:
#ifdef QGC_MOUSE_ENABLED_WIN
    /** @brief Get a motion input from 3DMouse */
    void motion3DMouse(std::vector<float> &motionData);
    /** @brief Get a button input from 3DMouse */
    void button3DMouseDown(int button);
#endif //QGC_MOUSE_ENABLED_WIN
#ifdef QGC_MOUSE_ENABLED_LINUX
    /** @brief Get an XEvent to check it for an 3DMouse event (motion or button) */
    void handleX11Event(XEvent* event);
#endif //QGC_MOUSE_ENABLED_LINUX

private slots:
    void _activeVehicleChanged(Vehicle* vehicle);
};

#endif // MOUSE6DOFINPUT_H
