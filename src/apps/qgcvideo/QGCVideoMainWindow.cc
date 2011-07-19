/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Implementation of main window
 *
 *   @author Dominik Honegger
 *
 */

#include "QGCVideoMainWindow.h"
#include "ui_QGCVideoMainWindow.h"

#include "UDPLink.h"
#include <QDebug>

  QByteArray imageRecBuffer = QByteArray(376*240,255);
  static int part = 0;

QGCVideoMainWindow::QGCVideoMainWindow(QWidget *parent) :
    QMainWindow(parent),
    link(QHostAddress::Any, 5555),
    ui(new Ui::QGCVideoMainWindow)
{
    ui->setupUi(this);

    // Set widgets in video mode
    ui->video1Widget->enableVideo(true);
    ui->video2Widget->enableVideo(true);
    ui->video3Widget->enableVideo(true);
    ui->video4Widget->enableVideo(true);

    // Connect link to this widget, receive all bytes
    connect(&link, SIGNAL(bytesReceived(LinkInterface*,QByteArray)), this, SLOT(receiveBytes(LinkInterface*,QByteArray)));

    // Open port
    link.connect();
}

QGCVideoMainWindow::~QGCVideoMainWindow()
{
    delete ui;
}

void QGCVideoMainWindow::receiveBytes(LinkInterface* link, QByteArray data)
{
    // There is no need to differentiate between links
    // for this use case here
    Q_UNUSED(link);

    // Image data is stored in QByteArray
    // Output bytes and load Lenna!

    QString bytes;
    QString index;
    QString ascii;



    // TODO FIXME Fabian
    // RAW hardcoded to 22x22
    int imgWidth = 376;
    int imgHeight = 240;
    int imgColors = 255;


    //const int headerSize = 15;

    // Construct PGM header
    QString header("P5\n%1 %2\n%3\n");
    header = header.arg(imgWidth).arg(imgHeight).arg(imgColors);

    switch (data[0])
    {
    case (1):
        {
            for (int i=4; i<data.size()/4; i++)
            {
                imageRecBuffer[i] = data[i*4];
                part = part | 1;
            }
        }
    case (2):
        {
            for (int i=4; i<data.size()/4; i++)
            {
                imageRecBuffer[i+45124/4*2] = data[i*4];
                part = part | 2;
            }
        }
//    case (3):
//        {
//            for (int i=4; i<data.size()/4; i++)
//            {
//                imageRecBuffer[i+45124/4*2] = data[i*4];
//                part = part | 4;
//            }
//        }
    }
    if(part==3)
    {
        for (int i=45124/4*3; i<376*240; i++)
        {
            imageRecBuffer[i] = 255;
        }
        QByteArray tmpImage(header.toStdString().c_str(), header.toStdString().size());
        tmpImage.append(imageRecBuffer);


        // Load image into window
        QImage test(":images/patterns/lenna.jpg");
        QImage image;


        if (imageRecBuffer.isNull())
            {
                qDebug()<< "could not convertToPGM()";

            }

            if (!image.loadFromData(tmpImage, "PGM"))
            {
                qDebug()<< "could not create extracted image";

            }

        tmpImage.clear();
        ui->video1Widget->copyImage(test);
        ui->video2Widget->copyImage(image);
        //ui->video3Widget->copyImage(test);
        //ui->video4Widget->copyImage(test);
        part = 0;
        imageRecBuffer.clear();
    }


    unsigned char i0 = data[0];
    index.append(QString().sprintf("%02x ", i0));

    for (int j=0; j<data.size(); j++) {
        unsigned char v = data[j];
        bytes.append(QString().sprintf("%02x ", v));
        if (data.at(j) > 31 && data.at(j) < 127)
        {
            ascii.append(data.at(j));
        }
        else
        {
            ascii.append(219);
        }

    }
     qDebug() << "Received" << data.size() << "bytes";
     qDebug() << "index: " <<index;
    //qDebug() << bytes;
    //qDebug() << "ASCII:" << ascii;




}
