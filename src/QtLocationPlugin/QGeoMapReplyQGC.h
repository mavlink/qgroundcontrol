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

#ifndef QGEOMAPREPLYQGC_H
#define QGEOMAPREPLYQGC_H

#include <QtNetwork/QNetworkReply>
#include <QtLocation/private/qgeotiledmapreply_p.h>
#include <QTimer>

#include "QGCMapEngineData.h"

class QGeoTiledMapReplyQGC : public QGeoTiledMapReply
{
    Q_OBJECT
public:
    QGeoTiledMapReplyQGC(QNetworkAccessManager*  networkManager, const QNetworkRequest& request, const QGeoTileSpec &spec, QObject *parent = 0);
    ~QGeoTiledMapReplyQGC();
    void abort();

signals:
    void terrainDone            (QByteArray responseBytes, QNetworkReply::NetworkError error);

private slots:
    void networkReplyFinished   ();
    void networkReplyError      (QNetworkReply::NetworkError error);
    void cacheReply             (QGCCacheTile* tile);
    void cacheError             (QGCMapTask::TaskType type, QString errorString);
    void timeout                ();

private:
    void _clearReply            ();

private:
    QNetworkReply*          _reply;
    QNetworkRequest         _request;
    QNetworkAccessManager*  _networkManager;
    QByteArray              _badMapbox;
    QByteArray              _badTile;
    QTimer                  _timer;
    static int              _requestCount;
};

#endif // QGEOMAPREPLYQGC_H
