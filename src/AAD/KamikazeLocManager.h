#pragma once

#include <QtCore/QObject>
#include <QGeoCoordinate>

class KamikazeLocManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate NOTIFY coordinateChanged)

public:
    static KamikazeLocManager* instance();

    Q_INVOKABLE void setCoordinate(QGeoCoordinate coord) {
        _Coordinate = coord;
        qDebug() << "Current Coordinate:" << _Coordinate;
        emit coordinateChanged();
    }

    QGeoCoordinate coordinate() const { return _Coordinate; }

signals:
    void coordinateChanged();

private:
    QGeoCoordinate _Coordinate;
};