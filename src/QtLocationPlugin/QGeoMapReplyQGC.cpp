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
#include "QGeoMapReplyQGC.h"
#include "QGeoTileFetcherQGC.h"

#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QFile>
#include "TerrainTile.h"

int QGeoTiledMapReplyQGC::_requestCount = 0;

//-----------------------------------------------------------------------------
QGeoTiledMapReplyQGC::QGeoTiledMapReplyQGC(QNetworkAccessManager *networkManager, const QNetworkRequest &request, const QGeoTileSpec &spec, QObject *parent)
    : QGeoTiledMapReply(spec, parent)
    , _reply(nullptr)
    , _request(request)
    , _networkManager(networkManager)
{
    if(_request.url().isEmpty()) {
        if(!_badMapbox.size()) {
            QFile b(":/res/notile.png");
            if(b.open(QFile::ReadOnly))
                _badMapbox = b.readAll();
        }
        setMapImageData(_badMapbox);
        setMapImageFormat("png");
        setFinished(true);
        setCached(false);
    } else {
        QGCFetchTileTask* task = getQGCMapEngine()->createFetchTileTask(getQGCMapEngine()->urlFactory()->getTypeFromId(spec.mapId()), spec.x(), spec.y(), spec.zoom());
        connect(task, &QGCFetchTileTask::tileFetched, this, &QGeoTiledMapReplyQGC::cacheReply);
        connect(task, &QGCMapTask::error, this, &QGeoTiledMapReplyQGC::cacheError);
        getQGCMapEngine()->addTask(task);
    }
}

//-----------------------------------------------------------------------------
QGeoTiledMapReplyQGC::~QGeoTiledMapReplyQGC()
{
    _clearReply();
}

//-----------------------------------------------------------------------------
void
QGeoTiledMapReplyQGC::_clearReply()
{
    _timer.stop();
    if (_reply) {
        _reply->deleteLater();
        _reply = nullptr;
        _requestCount--;
    }
}

//-----------------------------------------------------------------------------
void
QGeoTiledMapReplyQGC::abort()
{
    _timer.stop();
    if (_reply)
        _reply->abort();
    emit aborted();
}

//-----------------------------------------------------------------------------
void
QGeoTiledMapReplyQGC::networkReplyFinished()
{
    _timer.stop();
    if (!_reply) {
        emit aborted();
        return;
    }
    if (_reply->error() != QNetworkReply::NoError) {
        emit aborted();
        return;
    }
    QByteArray a = _reply->readAll();
    QString format = getQGCMapEngine()->urlFactory()->getImageFormat(tileSpec().mapId(), a);
    //-- Test for a specialized, elevation data (not map tile)
    if( getQGCMapEngine()->urlFactory()->isElevation(tileSpec().mapId())){
        a = TerrainTile::serialize(a);
        //-- Cache it if valid
        if(!a.isEmpty()) {
            getQGCMapEngine()->cacheTile(
                getQGCMapEngine()->urlFactory()->getTypeFromId(
                    tileSpec().mapId()),
                tileSpec().x(), tileSpec().y(), tileSpec().zoom(), a, format);
        }
        emit terrainDone(a, QNetworkReply::NoError);
    } else {
        //-- This is a map tile. Process and cache it if valid.
        setMapImageData(a);
        if(!format.isEmpty()) {
            setMapImageFormat(format);
            getQGCMapEngine()->cacheTile(getQGCMapEngine()->urlFactory()->getTypeFromId(tileSpec().mapId()), tileSpec().x(), tileSpec().y(), tileSpec().zoom(), a, format);
        }
        setFinished(true);
    }
    _clearReply();
}

//-----------------------------------------------------------------------------
void
QGeoTiledMapReplyQGC::networkReplyError(QNetworkReply::NetworkError error)
{
    _timer.stop();
    if (!_reply) {
        return;
    }
    //-- Test for a specialized, elevation data (not map tile)
    if( getQGCMapEngine()->urlFactory()->isElevation(tileSpec().mapId())){
        emit terrainDone(QByteArray(), error);
    } else {
        //-- Regular map tile
        if (error != QNetworkReply::OperationCanceledError) {
            qWarning() << "Fetch tile error:" << _reply->errorString();
            setError(QGeoTiledMapReply::CommunicationError, _reply->errorString());
        }
        setFinished(true);
    }
    _clearReply();
}

//-----------------------------------------------------------------------------
void
QGeoTiledMapReplyQGC::cacheError(QGCMapTask::TaskType type, QString /*errorString*/)
{
    if(!getQGCMapEngine()->isInternetActive()) {
        if( getQGCMapEngine()->urlFactory()->isElevation(tileSpec().mapId())){
            emit terrainDone(QByteArray(), QNetworkReply::NetworkSessionFailedError);
        } else {
            setError(QGeoTiledMapReply::CommunicationError, "Network not available");
            setFinished(true);
        }
    } else {
        if(type != QGCMapTask::taskFetchTile) {
            qWarning() << "QGeoTiledMapReplyQGC::cacheError() for wrong task";
        }
        //-- Tile not in cache. Get it off the Internet.
#if !defined(__mobile__)
        QNetworkProxy proxy = _networkManager->proxy();
        QNetworkProxy tProxy;
        tProxy.setType(QNetworkProxy::DefaultProxy);
        _networkManager->setProxy(tProxy);
#endif
        _reply = _networkManager->get(_request);
        _reply->setParent(nullptr);
        connect(_reply, &QNetworkReply::finished, this, &QGeoTiledMapReplyQGC::networkReplyFinished);
        connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkReplyError(QNetworkReply::NetworkError)));
#if !defined(__mobile__)
        _networkManager->setProxy(proxy);
#endif
        //- Wait for an answer up to 10 seconds
        connect(&_timer, &QTimer::timeout, this, &QGeoTiledMapReplyQGC::timeout);
        _timer.setSingleShot(true);
        _timer.start(10000);
        _requestCount++;
    }
}

//-----------------------------------------------------------------------------
void
QGeoTiledMapReplyQGC::cacheReply(QGCCacheTile* tile)
{
    //-- Test for a specialized, elevation data (not map tile)
    if( getQGCMapEngine()->urlFactory()->isElevation(tileSpec().mapId())){
        emit terrainDone(tile->img(), QNetworkReply::NoError);
    } else {
        //-- Regular map tile
        setMapImageData(tile->img());
        setMapImageFormat(tile->format());
        setFinished(true);
        setCached(true);
    }
    tile->deleteLater();
}

//-----------------------------------------------------------------------------
void
QGeoTiledMapReplyQGC::timeout()
{
    if(_reply) {
        _reply->abort();
    }
    emit aborted();
}
