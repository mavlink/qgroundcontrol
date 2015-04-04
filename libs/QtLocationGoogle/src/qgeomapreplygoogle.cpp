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
** Adapted for google maps with the intent of use for QGroundControl
**
** Gus Grubba <mavlink@grubba.com>
**
****************************************************************************/

#include <QtLocation/private/qgeotilespec_p.h>

#include "qgeomapreplygoogle.h"

QGeoMapReplyGoogle::QGeoMapReplyGoogle(QNetworkReply *reply, const QGeoTileSpec &spec, QObject *parent)
    : QGeoTiledMapReply(spec, parent)
    , m_reply(reply)
{
    connect(m_reply, SIGNAL(finished()),                         this, SLOT(networkReplyFinished()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkReplyError(QNetworkReply::NetworkError)));
    connect(m_reply, SIGNAL(destroyed()),                        this, SLOT(replyDestroyed()));
}

QGeoMapReplyGoogle::~QGeoMapReplyGoogle()
{
    if (m_reply) {
        m_reply->deleteLater();
        m_reply = 0;
    }
}

void QGeoMapReplyGoogle::abort()
{
    if (!m_reply)
        return;
    m_reply->abort();
}

QNetworkReply *QGeoMapReplyGoogle::networkReply() const
{
    return m_reply;
}

void QGeoMapReplyGoogle::replyDestroyed()
{
    m_reply = 0;
}

void QGeoMapReplyGoogle::networkReplyFinished()
{
    if (!m_reply)
        return;

    if (m_reply->error() != QNetworkReply::NoError)
        return;

    QByteArray a = m_reply->readAll();
    setMapImageData(a);

    if(tileSpec().mapId() > 0 && tileSpec().mapId() < 5)
        setMapImageFormat("png");
    else
        qWarning("Unknown map id %d", tileSpec().mapId());

    setFinished(true);
    m_reply->deleteLater();
    m_reply = 0;
}

void QGeoMapReplyGoogle::networkReplyError(QNetworkReply::NetworkError error)
{
    if (!m_reply)
        return;

    if (error != QNetworkReply::OperationCanceledError)
        setError(QGeoTiledMapReply::CommunicationError, m_reply->errorString());

    setFinished(true);
    m_reply->deleteLater();
    m_reply = 0;
}
