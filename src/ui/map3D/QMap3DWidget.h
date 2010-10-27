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
 *   @brief Definition of the class QMap3DWidget.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#ifndef QMAP3DWIDGET_H
#define QMAP3DWIDGET_H

#include <QLabel>

#include "Imagery.h"
#include "Q3DWidget.h"

class CheetahModel;
class UASInterface;

/**
 * @brief A 3D View widget which displays vehicle-centric information.
 **/
class QMap3DWidget : public Q3DWidget
{
    Q_OBJECT

public:
    explicit QMap3DWidget(QWidget* parent);
    ~QMap3DWidget();

    void buildLayout(void);

    static void display(void* clientData);
    void displayHandler(void);
//    void paintEvent(QPaintEvent *event);

    static void mouse(Qt::MouseButton button, MouseState state,
                      int32_t x, int32_t y, void* clientData);
    void mouseHandler(Qt::MouseButton button, MouseState state,
                      int32_t x, int32_t y);

    static void timer(void* clientData);
    void timerHandler(void);

    double getTime(void) const;

public slots:
    void setActiveUAS(UASInterface* uas);
    void markTarget(void);

private slots:
    void showGrid(int state);
    void showTrail(int state);
    void showImagery(const QString& text);
    void recenterCamera(void);
    void toggleLockCamera(int state);

protected:
    UASInterface* uas;
    void paintText(QString text, QColor color, float fontSize,
                   float refX, float refY, QPainter* painter) const;
    void drawWaypoints(void) const;

private:
    void drawPlatform(float roll, float pitch, float yaw) const;
    void drawGrid(float x, float y, float z) const;
    void drawImagery(double originX, double originY, double originZ,
                     const QString& zone, bool prefetch) const;
    void drawTrail(float x, float y, float z);
    void drawTarget(float x, float y, float z) const;

    void drawLegend(void);

    double lastRedrawTime;

    bool displayGrid;
    bool displayImagery;
    bool displayTrail;

    typedef struct
    {
        float x;
        float y;
        float z;
    } Pose3D;

    bool lockCamera;
    Pose3D lastUnlockedPose;
    bool updateLastUnlockedPose;
    Pose3D camOffset;

    QVarLengthArray<Pose3D, 10000> trail;

    bool displayTarget;
    bool displayWaypoints;
    Pose3D targetPosition;

    QScopedPointer<CheetahModel> cheetahModel;
    QScopedPointer<Imagery> imagery;

    QComboBox* imageryComboBox;
};

#endif // QMAP3DWIDGET_H
