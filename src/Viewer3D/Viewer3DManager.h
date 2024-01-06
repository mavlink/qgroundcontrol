#ifndef VIEWER3DMANAGER_H
#define VIEWER3DMANAGER_H

#include <QObject>

#include "Viewer3DQmlBackend.h"
#include "OsmParser.h"
#include "Viewer3DSettings.h"

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

// This class contains all the variables shared between the C++ and QML sides for 3D viewer.
class Viewer3DManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(OsmParser* osmParser MEMBER _osmParser CONSTANT)
    Q_PROPERTY(Viewer3DQmlBackend* qmlBackend MEMBER _qmlBackend CONSTANT)
    Q_PROPERTY(Viewer3DSettings* viewer3DSetting MEMBER _viewer3DSetting CONSTANT)

public:
    explicit Viewer3DManager();

protected:
    OsmParser *_osmParser = nullptr;
    Viewer3DQmlBackend *_qmlBackend = nullptr;
    Viewer3DSettings* _viewer3DSetting = nullptr;

};

#endif // VIEWER3DMANAGER_H
