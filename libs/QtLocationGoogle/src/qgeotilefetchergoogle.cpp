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

#include "qgeotilefetchergoogle.h"
#include "qgeomapreplygoogle.h"

#include <QtCore/QLocale>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtLocation/private/qgeotilespec_p.h>

QT_BEGIN_NAMESPACE

QGeoTileFetcherGoogle::QGeoTileFetcherGoogle(QGeoTiledMappingManagerEngine *parent)
    : QGeoTileFetcher(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_userAgent("Qt Application")
{
}

void QGeoTileFetcherGoogle::setUserAgent(const QByteArray &userAgent)
{
    m_userAgent = userAgent;
}

QGeoTiledMapReply *QGeoTileFetcherGoogle::getTileImage(const QGeoTileSpec &spec)
{
    QNetworkRequest request;
    request.setRawHeader("User-Agent", m_userAgent);

    QString url;
    if (spec.mapId() == 1) {
        url = QStringLiteral("http://mt1.google.com/vt/lyrs=m");
    } else if (spec.mapId() == 2) {
        url = QStringLiteral("http://mt1.google.com/vt/lyrs=s");
    } else if (spec.mapId() == 3) {
        url = QStringLiteral("http://mt1.google.com/vt/lyrs=p");
    } else if (spec.mapId() == 4) {
        url = QStringLiteral(" http://mt1.google.com/vt/lyrs=h");
    } else {
        qWarning("Unknown map id %d\n", spec.mapId());
        url = QStringLiteral("http://mt1.google.com/vt/lyrs=m");
    }

    url += QStringLiteral("&x=%1&y=%2&z=%3")
        .arg(spec.x())
        .arg(spec.y())
        .arg(spec.zoom());

    QStringList langs = QLocale::system().uiLanguages();
    if (langs.length() > 0) {
        url += QStringLiteral("&hl=%1").arg(langs[0]);
    }
    
    url += QStringLiteral("&scale=2");
    QUrl qurl(url);

    request.setUrl(qurl);
    QNetworkReply *reply = m_networkManager->get(request);
    reply->setParent(0);

    return new QGeoMapReplyGoogle(reply, spec);
}

QT_END_NAMESPACE
