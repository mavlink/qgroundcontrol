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
 *   @brief Implementation of CameraView
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include "CameraView.h"
#include <QDebug>

CameraView::CameraView(int width, int height, int depth, int channels, QWidget* parent) : QGLWidget(parent)
{
    rawImage = NULL;
    rawBuffer1 = NULL;
    rawBuffer2 = NULL;
    rawLastIndex = 0;
    image = NULL;
    imageStarted = false;
    // Init to black image
    //setImageSize(width, height, depth, channels);
    receivedWidth = width;
    receivedHeight = height;
    receivedDepth = depth;
    receivedChannels = channels;
    imageId = -1;

    // Fill with black background
    QImage fill = QImage(width, height, QImage::Format_Indexed8);
    fill.setNumColors(1);
    fill.setColor(0, qRgb(70, 200, 70));
    fill.fill(CameraView::initialColor);
    glImage = QGLWidget::convertToGLFormat(fill);

    resize(fill.size());

    // Set size once
    setFixedSize(fill.size());
    setMinimumSize(fill.size());
    setMaximumSize(fill.size());
    // Lock down the size
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
}

CameraView::~CameraView()
{
    delete rawImage;
    delete image;
}

void CameraView::addUAS(UASInterface* uas)
{
    // TODO Enable multi-uas support
    connect(uas, SIGNAL(imageStarted(int,int,int,int,int)), this, SLOT(startImage(int,int,int,int,int)));
    connect(uas, SIGNAL(imageDataReceived(int,const unsigned char*,int,int)), this, SLOT(setPixels(int,const unsigned char*,int,int)));
}

void CameraView::setImageSize(int width, int height, int depth, int channels)
{
    // Allocate raw image in correct size
    if (width != receivedWidth || height != receivedHeight || depth != receivedDepth || channels != receivedChannels || image == NULL) {
        // Set new size
        if (width > 0) receivedWidth  = width;
        if (height > 0) receivedHeight = height;
        if (depth > 1) receivedDepth = depth;
        if (channels > 1) receivedChannels = channels;

        rawExpectedBytes = (receivedWidth * receivedHeight * receivedDepth * receivedChannels) / 8;
        bytesPerLine = rawExpectedBytes / receivedHeight;
        // Delete old buffers if necessary
        rawImage = NULL;
        if (rawBuffer1 != NULL) delete rawBuffer1;
        if (rawBuffer2 != NULL) delete rawBuffer2;

        rawBuffer1 = (unsigned char*)malloc(rawExpectedBytes);
        rawBuffer2 = (unsigned char*)malloc(rawExpectedBytes);
        rawImage = rawBuffer1;
        // TODO check if old image should be deleted

        // Set image format
        // 8 BIT GREYSCALE IMAGE
        if (depth <= 8 && channels == 1) {
            image = new QImage(receivedWidth, receivedHeight, QImage::Format_Indexed8);
            // Create matching color table
            image->setNumColors(256);
            for (int i = 0; i < 256; i++) {
                image->setColor(i, qRgb(i, i, i));
                //qDebug() << __FILE__ << __LINE__ << std::hex << i;
            }

        }
        // 32 BIT COLOR IMAGE WITH ALPHA VALUES (#ARGB)
        else {
            image = new QImage(receivedWidth, receivedHeight, QImage::Format_ARGB32);
        }

        // Fill image with black pixels
        image->fill(CameraView::initialColor);
        glImage = QGLWidget::convertToGLFormat(*image);

        qDebug() << __FILE__ << __LINE__ << "Setting up image";

        // Set size once
        setFixedSize(receivedWidth, receivedHeight);
        setMinimumSize(receivedWidth, receivedHeight);
        setMaximumSize(receivedWidth, receivedHeight);
        // Lock down the size
        setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        resize(receivedWidth, receivedHeight);
    }

}

void CameraView::startImage(int imgid, int width, int height, int depth, int channels)
{
    this->imageId = imgid;
    //qDebug() << "CameraView: starting image (" << width << "x" << height << ", " << depth << "bits) with " << channels << "channels";

    // Copy previous image to screen if it hasn't been finished properly
    finishImage();

    // Reset image size if necessary
    setImageSize(width, height, depth, channels);
    imageStarted = true;
}

void CameraView::finishImage()
{
    if (imageStarted) {
        commitRawDataToGL();
        imageStarted = false;
    }
}

void CameraView::commitRawDataToGL()
{
    //qDebug() << __FILE__ << __LINE__ << "Copying raw data to GL buffer:" << rawImage << receivedWidth << receivedHeight << image->format();
    if (image != NULL) {
        QImage::Format format = image->format();
        QImage* newImage = new QImage(rawImage, receivedWidth, receivedHeight, format);
        if (format == QImage::Format_Indexed8) {
            // Create matching color table
            newImage->setNumColors(256);
            for (int i = 0; i < 256; i++) {
                newImage->setColor(i, qRgb(i, i, i));
                //qDebug() << __FILE__ << __LINE__ << std::hex << i;
            }
        }

        glImage = QGLWidget::convertToGLFormat(*newImage);
        delete image;
        image = newImage;
        // Switch buffers
        if (rawImage == rawBuffer1) {
            rawImage = rawBuffer2;
            //qDebug() << "Now buffer 2";
        } else {
            rawImage = rawBuffer1;
            //qDebug() << "Now buffer 1";
        }
    }
    paintGL();
}

void CameraView::saveImage(QString fileName)
{
    image->save(fileName);
}

void CameraView::saveImage()
{
    //Bring up popup
    QString fileName = "output.png";
    saveImage(fileName);
}

void CameraView::setPixels(int imgid, const unsigned char* imageData, int length, unsigned int startIndex)
{
    // FIXME imgid can be used to receive and then display multiple images
    // the image buffer should be converted into a n image buffer.
    Q_UNUSED(imgid);

    //    qDebug() << "at" << __FILE__ << __LINE__ << ": Received startindex" << startIndex << "and length" << length << "(" << startIndex+length << "of" << rawExpectedBytes << "bytes)";

    if (imageStarted) {
        //if (rawLastIndex != startIndex) qDebug() << "PACKET LOSS!";

        if (startIndex+length > rawExpectedBytes) {
            qDebug() << "CAMERAVIEW: OVERFLOW! startIndex:" << startIndex << "length:" << length << "image raw size" << ((receivedWidth * receivedHeight * receivedChannels * receivedDepth) / 8) - 1;
        } else {
            memcpy(rawImage+startIndex, imageData, length);

            rawLastIndex = startIndex+length;

            // Check if we just reached the end of the image
            if (startIndex+length == rawExpectedBytes) {
                //qDebug() << "CAMERAVIEW: END OF IMAGE REACHED!";
                finishImage();
                rawLastIndex = 0;
            }
        }

        //        for (int i = 0; i < length; i++)
        //        {
        //            for (int j = 0; j < receivedChannels; j++)
        //            {
        //                unsigned int x = (startIndex+i) % receivedWidth;
        //                unsigned int y = static_cast<unsigned int>((startIndex+i) / receivedWidth);
        //                qDebug() << "Setting pixel" << x << "," << y << "to" << (unsigned int)*(rawImage+startIndex+i);
        //            }
        //        }
    }
}

void CameraView::paintGL()
{
    glDrawPixels(glImage.width(), glImage.height(), GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());
}

void CameraView::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}
