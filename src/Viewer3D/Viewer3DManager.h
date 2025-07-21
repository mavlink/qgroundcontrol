/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class SettingsManager;
class OsmParser;
class Viewer3DQmlBackend;

// This class contains all the variables shared between the C++ and QML sides for 3D viewer.
class Viewer3DManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("OsmParser.h")
    Q_MOC_INCLUDE("Viewer3DQmlBackend.h")

    Q_PROPERTY(OsmParser* osmParser MEMBER _osmParser CONSTANT)
    Q_PROPERTY(Viewer3DQmlBackend* qmlBackend MEMBER _qmlBackend CONSTANT)

public:
    explicit Viewer3DManager();

    ~Viewer3DManager();

protected:
    OsmParser *_osmParser = nullptr;
    Viewer3DQmlBackend *_qmlBackend = nullptr;

};
