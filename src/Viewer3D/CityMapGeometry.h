
#ifndef CITYMAPGEOMETRY_H
#define CITYMAPGEOMETRY_H

#include <QQuick3DGeometry>
#include <QTimer>

#include "OsmParser.h"

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class CityMapGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    Q_PROPERTY(QString modelName READ modelName WRITE setModelName NOTIFY modelNameChanged)
    Q_PROPERTY(QString osmFilePath READ osmFilePath WRITE setOsmFilePath NOTIFY osmFilePathChanged)
    Q_PROPERTY(OsmParser* osmParser READ osmParser WRITE setOsmParser NOTIFY osmParserChanged)

public:

    CityMapGeometry();

    QString modelName() const { return _modelName; }
    void setModelName(QString modelName);

    QString osmFilePath() const {return _osmFilePath;}
    void setOsmFilePath(QString filePath);

    OsmParser* osmParser(){ return _osmParser;}
    void setOsmParser(OsmParser* newOsmParser);

    bool loadOsmMap();

signals:
    void modelNameChanged();
    void osmFilePathChanged();
    void osmParserChanged();

private:
    void updateViewer();

    QString _modelName;
    QString _osmFilePath;
    QByteArray _vertexData;
    OsmParser *_osmParser;
    bool _mapLoadedFlag;
};

#endif // CITYMAPGEOMETRY_H
