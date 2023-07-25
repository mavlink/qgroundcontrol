/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once
#include <QObject>
#include <QQmlListProperty>
#include <QQuickImageProvider>

#include "QGCToolbox.h"

// This is used to expose images from ImageProtocolHandler
class QGCImageProvider : public QGCTool, public QQuickImageProvider
{
public:
    QGCImageProvider        (QGCApplication* app, QGCToolbox* toolbox);
    ~QGCImageProvider       ();

    void    setImage        (QImage* pImage, int id = 0);

    // Overrdies from QQuickImageProvider
    QImage  requestImage    (const QString& id, QSize* size, const QSize& requestedSize) override;

    // Overrides from QGCTool
    void    setToolbox      (QGCToolbox *toolbox) override;

private:
    //-- TODO: For now this is holding a single image. If you happen to have two
    //   or more vehicles with flow, it will not work. To properly manage that condition
    //   this should be a map between each vehicle and its image. The URL provided
    //   for the image request would contain the vehicle identification.
    QImage _image;
};
