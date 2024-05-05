/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>

#include "Fact.h"
#include <QmlObjectListItem.h>

/// This class is used to encapsulate the QGeoCoordinate associated with a Rally Point into a QObject such
/// that it can be used in a QmlObjectListMode for Qml.
class RallyPoint : public QmlObjectListItem
{
    Q_OBJECT
    
public:
    RallyPoint(const QGeoCoordinate& coordinate, QObject* parent = nullptr);
    RallyPoint(const RallyPoint& other, QObject* parent = nullptr);

    ~RallyPoint();

    const RallyPoint& operator=(const RallyPoint& other);
    
    Q_PROPERTY(QGeoCoordinate   coordinate      READ coordinate     WRITE setCoordinate     NOTIFY coordinateChanged)
    Q_PROPERTY(QVariantList     textFieldFacts  MEMBER _textFieldFacts                      CONSTANT)

    QGeoCoordinate coordinate(void) const;
    void setCoordinate(const QGeoCoordinate& coordinate);

    static double getDefaultFactAltitude();

signals:
    void coordinateChanged      (const QGeoCoordinate& coordinate);

private slots:
    void _sendCoordinateChanged(void);

private:
    void _factSetup(void);
    static void _cacheFactMetadata();

    Fact _longitudeFact;
    Fact _latitudeFact;
    Fact _altitudeFact;

    QVariantList _textFieldFacts;

    static QMap<QString, FactMetaData*> _metaDataMap;

    static const char* _longitudeFactName;
    static const char* _latitudeFactName;
    static const char* _altitudeFactName;
};
