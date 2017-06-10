/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef AirMapManager_H
#define AirMapManager_H

#include "QGCToolbox.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

#include <QGeoCoordinate>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(AirMapManagerLog)


class AirspaceRestriction : public QObject
{
    Q_OBJECT

public:
    AirspaceRestriction(QObject* parent = NULL);
};

class PolygonAirspaceRestriction : public AirspaceRestriction
{
    Q_OBJECT

public:
    PolygonAirspaceRestriction(const QVariantList& polygon, QObject* parent = NULL);

    Q_PROPERTY(QVariantList polygon MEMBER _polygon CONSTANT)

private:
    QVariantList    _polygon;
};

class CircularAirspaceRestriction : public AirspaceRestriction
{
    Q_OBJECT

public:
    CircularAirspaceRestriction(const QGeoCoordinate& center, double radius, QObject* parent = NULL);

    Q_PROPERTY(QGeoCoordinate   center MEMBER _center CONSTANT)
    Q_PROPERTY(double           radius MEMBER _radius CONSTANT)

private:
    QGeoCoordinate  _center;
    double          _radius;
};


/// AirMap server communication support.
class AirMapManager : public QGCTool
{
    Q_OBJECT
    
public:
    AirMapManager(QGCApplication* app, QGCToolbox* toolbox);
    ~AirMapManager();

    /// Set the ROI for airspace information
    ///     @param center Center coordinate for ROI
    ///     @param radiusMeters Radius in meters around center which is of interest
    void setROI(QGeoCoordinate& center, double radiusMeters);

    QmlObjectListModel* polygonRestrictions(void) { return &_polygonList; }
    QmlObjectListModel* circularRestrictions(void) { return &_circleList; }
        
private slots:
    void _getFinished(void);
    void _getError(QNetworkReply::NetworkError code);
    void _updateToROI(void);

private:
    void _get(QUrl url);
    void _parseAirspaceJson(const QJsonDocument& airspaceDoc);

    enum class State {
        Idle,
        RetrieveList,
        RetrieveItems,
    };

    State                   _state = State::Idle;
    int                     _numAwaitingItems = 0;
    QGeoCoordinate          _roiCenter;
    double                  _roiRadius;
    QNetworkAccessManager   _networkManager;
    QTimer                  _updateTimer;
    QmlObjectListModel      _polygonList;
    QmlObjectListModel      _circleList;
    QList<PolygonAirspaceRestriction*> _nextPolygonList;
    QList<CircularAirspaceRestriction*> _nextcircleList;
};

#endif
