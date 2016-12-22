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
** Gus Grubba <mavlink@grubba.com>
**
****************************************************************************/

#include "QGCMapEngine.h"
#include "QGeoMapReplyQGC.h"

#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QFile>

//-----------------------------------------------------------------------------
QGeoTiledMapReplyQGC::QGeoTiledMapReplyQGC(QNetworkAccessManager *networkManager, const QNetworkRequest &request, const QGeoTileSpec &spec, QObject *parent)
    : QGeoTiledMapReply(spec, parent)
    , _reply(NULL)
    , _request(request)
    , _networkManager(networkManager)
{
    if(_request.url().isEmpty()) {
        if(!_badMapBox.size()) {
            QFile b(":/res/notile.png");
            if(b.open(QFile::ReadOnly))
                _badMapBox = b.readAll();
        }
        setMapImageData(_badMapBox);
        setMapImageFormat("png");
        setFinished(true);
        setCached(false);
    } else {
        QGCFetchTileTask* task = getQGCMapEngine()->createFetchTileTask((UrlFactory::MapType)spec.mapId(), spec.x(), spec.y(), spec.zoom());
        connect(task, &QGCFetchTileTask::tileFetched, this, &QGeoTiledMapReplyQGC::cacheReply);
        connect(task, &QGCMapTask::error, this, &QGeoTiledMapReplyQGC::cacheError);
        getQGCMapEngine()->addTask(task);
    }
}

//-----------------------------------------------------------------------------
QGeoTiledMapReplyQGC::~QGeoTiledMapReplyQGC()
{
    if (_reply) {
        _reply->deleteLater();
        _reply = 0;
    }
}
//-----------------------------------------------------------------------------
void
QGeoTiledMapReplyQGC::abort()
{
    if (_reply)
        _reply->abort();
}

//-----------------------------------------------------------------------------
void
QGeoTiledMapReplyQGC::replyDestroyed()
{
    _reply = 0;
}

//-----------------------------------------------------------------------------
void
QGeoTiledMapReplyQGC::networkReplyFinished()
{
    if (!_reply) {
        return;
    }
    if (_reply->error() != QNetworkReply::NoError) {
        return;
    }
    QByteArray a = _reply->readAll();
    setMapImageData(a);
    QString format = getQGCMapEngine()->urlFactory()->getImageFormat((UrlFactory::MapType)tileSpec().mapId(), a);
    if(!format.isEmpty()) {
        setMapImageFormat(format);
        getQGCMapEngine()->cacheTile((UrlFactory::MapType)tileSpec().mapId(), tileSpec().x(), tileSpec().y(), tileSpec().zoom(), a, format);
    }
    setFinished(true);
    _reply->deleteLater();
    _reply = 0;
}

//-----------------------------------------------------------------------------
void
QGeoTiledMapReplyQGC::networkReplyError(QNetworkReply::NetworkError error)
{
    if (!_reply) {
        return;
    }
    if (error != QNetworkReply::OperationCanceledError) {
        qWarning() << "Fetch tile error:" << _reply->errorString();
    }
    _reply->deleteLater();
    _reply = 0;
    if(!_badTile.size()) {
        QFile b(":/res/notile.png");
        if(b.open(QFile::ReadOnly))
            _badTile = b.readAll();
    }
    setMapImageData(_badTile);
    setMapImageFormat("png");
    setFinished(true);
    setCached(false);
}

//-----------------------------------------------------------------------------
void
QGeoTiledMapReplyQGC::cacheError(QGCMapTask::TaskType type, QString /*errorString*/)
{
    if(type != QGCMapTask::taskFetchTile) {
        qWarning() << "QGeoTiledMapReplyQGC::cacheError() for wrong task";
    }
    //-- Tile not in cache. Get it off the Internet.
    QNetworkProxy proxy = _networkManager->proxy();
    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    _networkManager->setProxy(tProxy);
    _reply = _networkManager->get(_request);
    _reply->setParent(0);
    connect(_reply, SIGNAL(finished()),                         this, SLOT(networkReplyFinished()));
    connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkReplyError(QNetworkReply::NetworkError)));
    connect(_reply, SIGNAL(destroyed()),                        this, SLOT(replyDestroyed()));
    _networkManager->setProxy(proxy);
}

//-----------------------------------------------------------------------------
void
QGeoTiledMapReplyQGC::cacheReply(QGCCacheTile* tile)
{
    setMapImageData(tile->img());
    setMapImageFormat(tile->format());
    setFinished(true);
    setCached(true);
    tile->deleteLater();
}
