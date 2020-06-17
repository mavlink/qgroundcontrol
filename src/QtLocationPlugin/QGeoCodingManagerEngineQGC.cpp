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

#include "QGeoCodingManagerEngineQGC.h"
#include "QGeoCodeReplyQGC.h"

#include <QtCore/QVariantMap>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QLocale>
#include <QDebug>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoAddress>
#include <QtPositioning/QGeoShape>
#include <QtPositioning/QGeoRectangle>

static QString addressToQuery(const QGeoAddress &address)
{
    return address.street()     + QStringLiteral(", ") +
           address.district()   + QStringLiteral(", ") +
           address.city()       + QStringLiteral(", ") +
           address.state()      + QStringLiteral(", ") +
           address.country();
}

static QString boundingBoxToLtrb(const QGeoRectangle &rect)
{
    return QString::number(rect.topLeft().longitude())      + QLatin1Char(',') +
           QString::number(rect.topLeft().latitude())       + QLatin1Char(',') +
           QString::number(rect.bottomRight().longitude())  + QLatin1Char(',') +
           QString::number(rect.bottomRight().latitude());
}

QGeoCodingManagerEngineQGC::QGeoCodingManagerEngineQGC(
    const QVariantMap &parameters,
    QGeoServiceProvider::Error *error,
    QString *errorString)
    : QGeoCodingManagerEngine(parameters), m_networkManager(new QNetworkAccessManager(this))
{
    if (parameters.contains(QStringLiteral("useragent")))
        m_userAgent = parameters.value(QStringLiteral("useragent")).toString().toLatin1();
    else
        m_userAgent = "Qt Location based application";
    *error = QGeoServiceProvider::NoError;
    errorString->clear();
}

QGeoCodingManagerEngineQGC::~QGeoCodingManagerEngineQGC()
{
}

QGeoCodeReply *QGeoCodingManagerEngineQGC::geocode(const QGeoAddress &address, const QGeoShape &bounds)
{
    return geocode(addressToQuery(address), -1, -1, bounds);
}

QGeoCodeReply *QGeoCodingManagerEngineQGC::geocode(const QString &address, int limit, int offset, const QGeoShape &bounds)
{
    Q_UNUSED(limit);
    Q_UNUSED(offset);

    QNetworkRequest request;
    request.setRawHeader("User-Agent", m_userAgent);

    QUrl url(QStringLiteral("http://maps.googleapis.com/maps/api/geocode/json"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("sensor"), QStringLiteral("false"));
    query.addQueryItem(QStringLiteral("language"), locale().name().left(2));
    query.addQueryItem(QStringLiteral("address"), address);
    if (bounds.type() == QGeoShape::RectangleType) {
        query.addQueryItem(QStringLiteral("bounds"), boundingBoxToLtrb(bounds));
    }

    url.setQuery(query);
    request.setUrl(url);
    //qDebug() << url;

    QNetworkReply *reply = m_networkManager->get(request);
    reply->setParent(0);

    QGeoCodeReplyQGC *geocodeReply = new QGeoCodeReplyQGC(reply);

    connect(geocodeReply, &QGeoCodeReply::finished, this, &QGeoCodingManagerEngineQGC::replyFinished);
    connect(geocodeReply, SIGNAL(error(QGeoCodeReply::Error,QString)),
            this, SLOT(replyError(QGeoCodeReply::Error,QString)));

    return geocodeReply;
}

QGeoCodeReply *QGeoCodingManagerEngineQGC::reverseGeocode(const QGeoCoordinate &coordinate, const QGeoShape &bounds)
{
    Q_UNUSED(bounds)

    QNetworkRequest request;
    request.setRawHeader("User-Agent", m_userAgent);

    QUrl url(QStringLiteral("http://maps.googleapis.com/maps/api/geocode/json"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("sensor"), QStringLiteral("false"));
    query.addQueryItem(QStringLiteral("language"), locale().name().left(2));
    query.addQueryItem(QStringLiteral("latlng"), QStringLiteral("%1,%2")
                       .arg(coordinate.latitude())
                       .arg(coordinate.longitude()));

    url.setQuery(query);
    request.setUrl(url);
    //qDebug() << url;

    QNetworkReply *reply = m_networkManager->get(request);
    reply->setParent(0);

    QGeoCodeReplyQGC *geocodeReply = new QGeoCodeReplyQGC(reply);

    connect(geocodeReply, &QGeoCodeReply::finished, this, &QGeoCodingManagerEngineQGC::replyFinished);
    connect(geocodeReply, SIGNAL(error(QGeoCodeReply::Error,QString)),
            this, SLOT(replyError(QGeoCodeReply::Error,QString)));

    return geocodeReply;
}

void QGeoCodingManagerEngineQGC::replyFinished()
{
    QGeoCodeReply *reply = qobject_cast<QGeoCodeReply *>(sender());
    if (reply)
        emit finished(reply);
}

void QGeoCodingManagerEngineQGC::replyError(QGeoCodeReply::Error errorCode, const QString &errorString)
{
    QGeoCodeReply *reply = qobject_cast<QGeoCodeReply *>(sender());
    if (reply)
        emit error(reply, errorCode, errorString);
}
