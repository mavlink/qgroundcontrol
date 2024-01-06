#ifndef Viewer3DQmlBackend_H
#define Viewer3DQmlBackend_H

#include <QObject>
#include <qqml.h>
#include <QString>

#include "OsmParser.h"
#include "Viewer3DSettings.h"

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class Viewer3DQmlBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString osmFilePath MEMBER _osmFilePath NOTIFY cityMapPathChanged)
    Q_PROPERTY(QGeoCoordinate gpsRef READ gpsRef NOTIFY gpsRefChanged)
    Q_PROPERTY(float altitudeBias MEMBER _altitudeBias NOTIFY altitudeBiasChanged)

public:
    explicit Viewer3DQmlBackend(QObject *parent = nullptr);

    void init(Viewer3DSettings* viewerSettingThr,  OsmParser* osmThr=nullptr);

    QGeoCoordinate gpsRef(){return _gpsRef;}
    void setGpsRef(const QGeoCoordinate& gpsRef);

signals:
    void gpsRefChanged();
    void altitudeBiasChanged();
    void cityMapPathChanged();

private:
    OsmParser *_osmParserThread;

    QString _osmFilePath;
    QGeoCoordinate _gpsRef;
    float _altitudeBias;

protected slots:
    void _gpsRefChangedEvent(QGeoCoordinate newGpsRef);
};

#endif // Viewer3DQmlBackend_H
