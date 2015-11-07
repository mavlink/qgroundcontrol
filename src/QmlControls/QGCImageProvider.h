/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Image Provider
 *
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */


#ifndef QGCIMAGEPROVIDER_H
#define QGCIMAGEPROVIDER_H

#include <QObject>
#include <QQmlListProperty>
#include <QQuickImageProvider>

#include "QGCToolbox.h"

class QGCImageProvider : public QGCTool, public QQuickImageProvider
{
public:
    QGCImageProvider        (QGCApplication* app);
    ~QGCImageProvider       ();
    QImage  requestImage    (const QString & id, QSize * size, const QSize & requestedSize);
    void    setImage        (QImage* pImage, int id = 0);
    void    setToolbox      (QGCToolbox *toolbox);
private:
    //-- TODO: For now this is holding a single image. If you happen to have two
    //   or more vehicles with flow, it will not work. To properly manage that condition
    //   this should be a map between each vehicle and its image. The URL provided
    //   for the image request would contain the vehicle identification.
    QImage _pImage;
};


#endif // QGCIMAGEPROVIDER_H
