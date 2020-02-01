/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Image Provider
 *
 *   @author Gus Grubba <gus@auterion.com>
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
    QGCImageProvider        (QGCApplication* app, QGCToolbox* toolbox);
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
