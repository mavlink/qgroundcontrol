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

#include <QtCore/QLocale>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtLocation/private/qgeotilespec_p.h>

#include "qgeotilefetcherqgc.h"
#include "qgeomapreplyqgc.h"

QGeoTileFetcherQGC::QGeoTileFetcherQGC(QGeoTiledMappingManagerEngine *parent)
    : QGeoTileFetcher(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_userAgent("Qt Application")
    , m_UrlFactory(NULL)
{
    QStringList langs = QLocale::system().uiLanguages();
    if (langs.length() > 0) {
        m_Language = langs[0];
    }
    m_UrlFactory = new OpenPilot::UrlFactory(m_networkManager);
}

QGeoTileFetcherQGC::~QGeoTileFetcherQGC()
{
    if(m_UrlFactory)
        delete m_UrlFactory;
}

void QGeoTileFetcherQGC::setUserAgent(const QByteArray &userAgent)
{
    m_userAgent = userAgent;
}

QGeoTiledMapReply *QGeoTileFetcherQGC::getTileImage(const QGeoTileSpec &spec)
{
    QNetworkRequest request;
    QString url = m_UrlFactory->makeImageUrl((OpenPilot::MapType)spec.mapId(), QPoint(spec.x(), spec.y()), spec.zoom(), m_Language);

    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", m_userAgent);
    request.setRawHeader("Accept", "*/*");
    switch ((OpenPilot::MapType)spec.mapId()) {
    case OpenPilot::GoogleMap:
    case OpenPilot::GoogleSatellite:
    case OpenPilot::GoogleLabels:
    case OpenPilot::GoogleTerrain:
    case OpenPilot::GoogleHybrid:
    {
        request.setRawHeader("Referrer", "http://maps.google.com/");
    }
    break;
    case OpenPilot::GoogleMapChina:
    case OpenPilot::GoogleSatelliteChina:
    case OpenPilot::GoogleLabelsChina:
    case OpenPilot::GoogleTerrainChina:
    case OpenPilot::GoogleHybridChina:
    {
        request.setRawHeader("Referrer", "http://ditu.google.cn/");
    }
    break;
    case OpenPilot::BingHybrid:
    case OpenPilot::BingMap:
    case OpenPilot::BingSatellite:
    {
        request.setRawHeader("Referrer", "http://www.bing.com/maps/");
    }
    break;
    case OpenPilot::YahooHybrid:
    case OpenPilot::YahooLabels:
    case OpenPilot::YahooMap:
    case OpenPilot::YahooSatellite:
    {
        request.setRawHeader("Referrer", "http://maps.yahoo.com/");
    }
    break;
    case OpenPilot::ArcGIS_MapsLT_Map_Labels:
    case OpenPilot::ArcGIS_MapsLT_Map:
    case OpenPilot::ArcGIS_MapsLT_OrtoFoto:
    case OpenPilot::ArcGIS_MapsLT_Map_Hybrid:
    {
        request.setRawHeader("Referrer", "http://www.maps.lt/map_beta/");
    }
    break;
    case OpenPilot::OpenStreetMapSurfer:
    case OpenPilot::OpenStreetMapSurferTerrain:
    {
        request.setRawHeader("Referrer", "http://www.mapsurfer.net/");
    }
    break;
    case OpenPilot::OpenStreetMap:
    case OpenPilot::OpenStreetOsm:
    {
        request.setRawHeader("Referrer", "http://www.openstreetmap.org/");
    }
    break;
    case OpenPilot::YandexMapRu:
    {
        request.setRawHeader("Referrer", "http://maps.yandex.ru/");
    }
    break;
    default:
        break;
    }

    QNetworkReply *reply = m_networkManager->get(request);
    reply->setParent(0);
    return new QGeoMapReplyQGC(reply, spec);

}
