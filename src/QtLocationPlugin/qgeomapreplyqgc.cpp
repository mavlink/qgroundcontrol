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

#include <QtLocation/private/qgeotilespec_p.h>

#include "qgeomapreplyqgc.h"
#include "OpenPilotMaps.h"

QGeoMapReplyQGC::QGeoMapReplyQGC(QNetworkReply *reply, const QGeoTileSpec &spec, QObject *parent)
    : QGeoTiledMapReply(spec, parent)
    , m_reply(reply)
{
    if(!reply)
    {
        setError(QGeoTiledMapReply::UnknownError, "Invalid tile request");
        setFinished(true);
    }
    else
    {
        connect(m_reply, SIGNAL(finished()),                         this, SLOT(networkReplyFinished()));
        connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkReplyError(QNetworkReply::NetworkError)));
        connect(m_reply, SIGNAL(destroyed()),                        this, SLOT(replyDestroyed()));
    }
}

QGeoMapReplyQGC::~QGeoMapReplyQGC()
{
    if (m_reply) {
        m_reply->deleteLater();
        m_reply = 0;
    }
}

void QGeoMapReplyQGC::abort()
{
    if (!m_reply)
        return;
    m_reply->abort();
}

QNetworkReply *QGeoMapReplyQGC::networkReply() const
{
    return m_reply;
}

void QGeoMapReplyQGC::replyDestroyed()
{
    m_reply = 0;
}

void QGeoMapReplyQGC::networkReplyFinished()
{
    if (!m_reply)
    {
        qWarning() << "NULL Map request reply";
        return;
    }

    if (m_reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "Map request reply error:" << m_reply->error();
        return;
    }

    QByteArray a = m_reply->readAll();
    setMapImageData(a);

    if(a.size() > 2)
    {
        if((char)a[0] == (char)0xff && (char)a[1] == (char)0xd8)
            setMapImageFormat("jpg");
        else if((char)a[0] == (char)0x89 && (char)a[1] == (char)0x50)
            setMapImageFormat("png");
        else
        {
            switch ((OpenPilot::MapType)tileSpec().mapId()) {
                case OpenPilot::GoogleMap:
                case OpenPilot::GoogleLabels:
                case OpenPilot::GoogleTerrain:
                case OpenPilot::GoogleHybrid:
                case OpenPilot::BingMap:
                case OpenPilot::OpenStreetMap:
                    setMapImageFormat("png");
                    break;
                case OpenPilot::GoogleSatellite:
                case OpenPilot::BingSatellite:
                case OpenPilot::BingHybrid:
                    setMapImageFormat("jpg");
                    break;
                default:
                    qWarning("Unknown map id %d", tileSpec().mapId());
                    break;
            }
        }
    }

    setFinished(true);
    m_reply->deleteLater();
    m_reply = 0;
}

void QGeoMapReplyQGC::networkReplyError(QNetworkReply::NetworkError error)
{
    if (!m_reply)
    {
        qWarning() << "NULL Map request error";
        return;
    }

    if (error != QNetworkReply::OperationCanceledError)
    {
        setError(QGeoTiledMapReply::CommunicationError, m_reply->errorString());
        qWarning() << "Map request reply error:" << m_reply->errorString();
    }

    setFinished(true);
    m_reply->deleteLater();
    m_reply = 0;
}
