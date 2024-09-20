/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QString>
#include <QtQuick3D/QQuick3DGeometry>

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class Viewer3DSettings;
class OsmParser;

class CityMapGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    Q_MOC_INCLUDE("OsmParser.h")

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
