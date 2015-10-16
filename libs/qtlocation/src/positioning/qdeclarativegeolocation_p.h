/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtPositioning module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEGEOLOCATION_P_H
#define QDECLARATIVEGEOLOCATION_P_H

#include <QtCore/QObject>
#include <QtPositioning/QGeoLocation>
#include <QtPositioning/QGeoRectangle>
#include <QtPositioning/private/qdeclarativegeoaddress_p.h>

QT_BEGIN_NAMESPACE

class Q_POSITIONING_EXPORT QDeclarativeGeoLocation : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QGeoLocation location READ location WRITE setLocation)
    Q_PROPERTY(QDeclarativeGeoAddress *address READ address WRITE setAddress NOTIFY addressChanged)
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)
    Q_PROPERTY(QGeoRectangle boundingBox READ boundingBox WRITE setBoundingBox NOTIFY boundingBoxChanged)

public:
    explicit QDeclarativeGeoLocation(QObject *parent = 0);
    explicit QDeclarativeGeoLocation(const QGeoLocation &src, QObject *parent = 0);
    ~QDeclarativeGeoLocation();

    QGeoLocation location() const;
    void setLocation(const QGeoLocation &src);

    QDeclarativeGeoAddress *address() const;
    void setAddress(QDeclarativeGeoAddress *address);
    QGeoCoordinate coordinate() const;
    void setCoordinate(const QGeoCoordinate coordinate);

    QGeoRectangle boundingBox() const;
    void setBoundingBox(const QGeoRectangle &boundingBox);

Q_SIGNALS:
    void addressChanged();
    void coordinateChanged();
    void boundingBoxChanged();

private:
    QDeclarativeGeoAddress *m_address;
    QGeoRectangle m_boundingBox;
    QGeoCoordinate m_coordinate;
};

QT_END_NAMESPACE

#endif // QDECLARATIVELOCATION_P_H
