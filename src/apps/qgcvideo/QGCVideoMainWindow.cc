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

  QByteArray imageRecBuffer1 = QByteArray(376*240,255);
  QByteArray imageRecBuffer2 = QByteArray(376*240,255);
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

    unsigned char i0 = data[0];

    switch (i0)
    {
    case 0x01:
        {
            for (int i=4; i<data.size()/4; i++)
            {
                imageRecBuffer1[i] = data[i*4];
                imageRecBuffer2[i] = data[i*4+1];
            }
            part = part | 1;
            break;
        }
    case 0x02:
        {
            for (int i=4; i<data.size()/4; i++)
            {
                imageRecBuffer1[i+45124/4] = data[i*4];
                imageRecBuffer2[i+45124/4] = data[i*4+1];
            }
            part = part | 2;
            break;
        }
    case 0x03:
        {
            for (int i=4; i<data.size()/4; i++)
            {
                imageRecBuffer1[i+45124/4*2] = data[i*4];
                imageRecBuffer2[i+45124/4*2] = data[i*4+1];
            }
            part = part | 4;
            break;
        }
    case 0x04:
        {
            for (int i=4; i<data.size()/4; i++)
            {
                imageRecBuffer1[i+45124/4*3] = data[i*4];
                imageRecBuffer2[i+45124/4*3] = data[i*4+1];
            }
            part = part | 8;
            break;
        }
    case 0x05:
        {
            for (int i=4; i<data.size()/4; i++)
            {
                imageRecBuffer1[i+45124/4*4] = data[i*4];
                imageRecBuffer2[i+45124/4*4] = data[i*4+1];
            }
            part = part | 16;
            break;
        }
    case 0x06:
        {
            for (int i=4; i<data.size()/4; i++)
            {
                imageRecBuffer1[i+45124/4*5] = data[i*4];
                imageRecBuffer2[i+45124/4*5] = data[i*4+1];
            }
            part = part | 32;
            break;
        }
    case 0x07:
        {
            for (int i=4; i<data.size()/4; i++)
            {
                imageRecBuffer1[i+45124/4*6] = data[i*4];
                imageRecBuffer2[i+45124/4*6] = data[i*4+1];
            }
            part = part | 64;
            break;
        }
    case 0x08:
        {
            for (int i=4; i<data.size()/4; i++)
            {
                imageRecBuffer1[i+45124/4*7] = data[i*4];
                imageRecBuffer2[i+45124/4*7] = data[i*4+1];
            }
            part = part | 128;
            break;
        }
    }
    if(part==255)
    {

        QByteArray tmpImage1(header.toStdString().c_str(), header.toStdString().size());
        tmpImage1.append(imageRecBuffer1);
        QByteArray tmpImage2(header.toStdString().c_str(), header.toStdString().size());
        tmpImage2.append(imageRecBuffer2);

        // Load image into window
        //QImage test(":images/patterns/lenna.jpg");
        QImage image1;
        QImage image2;


        if (imageRecBuffer1.isNull())
            {
                qDebug()<< "could not convertToPGM()";

            }

            if (!image1.loadFromData(tmpImage1, "PGM"))
            {
                qDebug()<< "could not create extracted image1";

            }
         if (imageRecBuffer2.isNull())
            {
                qDebug()<< "could not convertToPGM()";

            }

            if (!image2.loadFromData(tmpImage2, "PGM"))
            {
                 qDebug()<< "could not create extracted image2";

            }
        tmpImage1.clear();
        tmpImage2.clear();
        //ui->video1Widget->copyImage(test);
        ui->video2Widget->copyImage(image1);
        ui->video3Widget->copyImage(image2);
        //ui->video4Widget->copyImage(test);
        part = 0;
        imageRecBuffer1.clear();
        imageRecBuffer2.clear();
    }



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
