#pragma once

#include "UnitTest.h"

class QTemporaryDir;

class ShapeTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testLoadPolylineFromSHP();
    void _testLoadPolylineFromKML();
    void _testLoadPolygonFromSHP();
    void _testLoadPolygonFromKML();

private:
    static QString _copyRes(const QTemporaryDir &tmpDir, const QString &name);
};
