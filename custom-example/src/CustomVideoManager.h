/****************************************************************************
 *
 *   (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QTimer>
#include <QTime>
#include <QUrl>

#include "VideoManager.h"

class CustomVideoManager : public VideoManager
{
    Q_OBJECT
public:
    CustomVideoManager      (QGCApplication* app, QGCToolbox* toolbox);

protected:
    void _updateSettings    ();

};
