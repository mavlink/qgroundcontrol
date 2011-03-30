/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Brief Description
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include <QImage>
#include <QGLWidget>
#include "UASInterface.h"

class CameraView : public QGLWidget
{
    Q_OBJECT
public:
    CameraView(int width = 640, int height = 480, int depth = 8, int channels = 1, QWidget* parent = NULL);
    ~CameraView();

    void setImageSize(int width, int height, int depth, int channels);
    void paintGL();
    void resizeGL(int w, int h);

public slots:
    void addUAS(UASInterface* uas);
    void startImage(int imgid, int width, int height, int depth, int channels);
    void setPixels(int imgid, const unsigned char* imageData, int length, unsigned int startIndex);
    void finishImage();
    void saveImage();
    void saveImage(QString fileName);

protected:
    // Image buffers
    unsigned char* rawBuffer1;
    unsigned char* rawBuffer2;
    unsigned char* rawImage;
    unsigned int rawLastIndex;
    unsigned int rawExpectedBytes;
    unsigned int bytesPerLine;
    bool imageStarted;
    static const unsigned char initialColor = 0;
    QImage* image; ///< Double buffer image
    QImage glImage; ///< Displayed image
    int receivedDepth;
    int receivedChannels;
    int receivedWidth;
    int receivedHeight;
    QMap<int, QImage*> images; ///< Reference to the received images
    int imageId; ///< ID of the currently displayed image

    void commitRawDataToGL();
};

#endif // CAMERAVIEW_H
