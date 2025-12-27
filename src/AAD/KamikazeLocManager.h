#pragma once

#include <QtCore/QObject>
#include <QGeoCoordinate>

class KamikazeLocManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)

public:
    static KamikazeLocManager* instance();

    Q_INVOKABLE void setCoordinate(QGeoCoordinate coord);

    QGeoCoordinate coordinate() const { return _coordinate; }

signals:
    void coordinateChanged();

private:
    QGeoCoordinate _coordinate;
};
