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
    QString ascii;
    for (int i=0; i<data.size(); i++) {
        unsigned char v = data[i];
        bytes.append(QString().sprintf("%02x ", v));
        if (data.at(i) > 31 && data.at(i) < 127)
        {
            ascii.append(data.at(i));
        }
        else
        {
            ascii.append(219);
        }
    }
    qDebug() << "Received" << data.size() << "bytes";
    qDebug() << bytes;
    qDebug() << "ASCII:" << ascii;


    // Load image into window
    QImage test(":images/patterns/lenna.jpg");

    ui->video1Widget->copyImage(test);
    ui->video2Widget->copyImage(test);
    ui->video3Widget->copyImage(test);
    ui->video4Widget->copyImage(test);
}
