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
#ifdef MOUSE_ENABLED_WIN
#include "Mouse3DInput.h"
#endif //MOUSE_ENABLED_WIN

#include "UASInterface.h"

class Mouse6dofInput : public QThread
{
    Q_OBJECT

public:
#ifdef MOUSE_ENABLED_WIN
    Mouse6dofInput(Mouse3DInput* mouseInput);
#endif //MOUSE_ENABLED_WIN
#ifdef MOUSE_ENABLED_LINUX
    Mouse6dofInput(QWidget* parent);
    void init3dMouse(QWidget* parent);
#endif //MOUSE_ENABLED_LINUX

    ~Mouse6dofInput();
    void run();

    const double mouse3DMax;

protected:
    void init();

    UASInterface* uas;
    bool done;
    bool mouseActive;

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

public slots:
    void setActiveUAS(UASInterface* uas);
    void motion3DMouse(std::vector<float> &motionData);

};

#endif // MOUSE6DOFINPUT_H
