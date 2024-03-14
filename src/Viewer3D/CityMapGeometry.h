
#ifndef CITYMAPGEOMETRY_H
#define CITYMAPGEOMETRY_H

#include <QQuick3DGeometry>
#include <QTimer>

#include "OsmParser.h"

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class Viewer3DSettings;

class CityMapGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    Q_PROPERTY(QString modelName READ modelName WRITE setModelName NOTIFY modelNameChanged)
    Q_PROPERTY(OsmParser* osmParser READ osmParser WRITE setOsmParser NOTIFY osmParserChanged)

public:

    CityMapGeometry();

    QString modelName() const { return _modelName; }
    void setModelName(QString modelName);

    QString osmFilePath() const {return _osmFilePath;}

    OsmParser* osmParser(){ return _osmParser;}
    void setOsmParser(OsmParser* newOsmParser);

    bool loadOsmMap();

signals:
    void modelNameChanged();
    void osmFilePathChanged();
    void osmParserChanged();

private:
    void updateViewer();
    void clearViewer();

    QString _modelName;
    QString _osmFilePath;
    QByteArray _vertexData;
    OsmParser *_osmParser;
    bool _mapLoadedFlag;
    Viewer3DSettings* _viewer3DSettings = nullptr;

private slots:
    void setOsmFilePath(QVariant value);


};

#endif // CITYMAPGEOMETRY_H
