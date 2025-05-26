/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"

class MultiSignalSpyV2;
class QGCMapPolyline;
class QmlObjectListModel;
class QTemporaryDir;

class QGCMapPolylineTest : public UnitTest
{
    Q_OBJECT

protected:
    void init() final;

private slots:
    void _testDirty();
    void _testVertexManipulation();
    void _testShapeLoad();
    void _testSelectVertex();

private:
    QString _copyRes(const QTemporaryDir &tmpDir, const QString &name);

    MultiSignalSpyV2 *_multiSpyPolyline = nullptr;
    int _countChangedMask = 0;
    int _pathChangedMask = 0;
    int _dirtyChangedMask = 0;
    int _clearedMask = 0;
    int _isEmptyChangedMask = 0;
    int _isValidChangedMask = 0;
    MultiSignalSpyV2 *_multiSpyModel = nullptr;
    int _modelCountChangedMask = 0;
    int _modelDirtyChangedMask = 0;
    QGCMapPolyline *_mapPolyline = nullptr;
    QmlObjectListModel *_pathModel = nullptr;
    const QList<QGeoCoordinate> _linePoints = {
        QGeoCoordinate(47.635638361473475, -122.09269407980834),
        QGeoCoordinate(47.635638361473475, -122.08545246602667),
        QGeoCoordinate(47.63057923872075, -122.08545246602667),
        QGeoCoordinate(47.63057923872075, -122.09269407980834)
    };
};
