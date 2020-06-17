/****************************************************************************
**
** Copyright (C) 2013 Aaron McCarthy <mccarthy.aaron@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtLocation module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
** 2015.4.4
** Adapted for use with QGroundControl
**
** Gus Grubba <gus@auterion.com>
**
****************************************************************************/

#include "QGCMapEngine.h"
#include "QGeoTileFetcherQGC.h"
#include "QGeoMapReplyQGC.h"

#include <QtCore/QLocale>
#include <QtNetwork/QNetworkRequest>
#include <QtLocation/private/qgeotilespec_p.h>

//-----------------------------------------------------------------------------
QGeoTileFetcherQGC::QGeoTileFetcherQGC(QGeoTiledMappingManagerEngine *parent)
    : QGeoTileFetcher(parent)
    , _networkManager(new QNetworkAccessManager(this))
{
    //-- Check internet status every 30 seconds or so
    connect(&_timer, &QTimer::timeout, this, &QGeoTileFetcherQGC::timeout);
    _timer.setSingleShot(false);
    _timer.start(30000);
}

//-----------------------------------------------------------------------------
QGeoTileFetcherQGC::~QGeoTileFetcherQGC()
{

}

//-----------------------------------------------------------------------------
QGeoTiledMapReply*
QGeoTileFetcherQGC::getTileImage(const QGeoTileSpec &spec)
{
    //-- Build URL
    QNetworkRequest request = getQGCMapEngine()->urlFactory()->getTileURL(spec.mapId(), spec.x(), spec.y(), spec.zoom(), _networkManager);
    if ( ! request.url().isEmpty() ) {
        return new QGeoTiledMapReplyQGC(_networkManager, request, spec);
    }
    else {
        return nullptr;
    }
}

//-----------------------------------------------------------------------------
void
QGeoTileFetcherQGC::timeout()
{
    getQGCMapEngine()->testInternet();
}
