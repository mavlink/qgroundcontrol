#pragma once

#include <functional>
#include <memory>

#include "UnitTest.h"

class QGCTileCacheDatabase;
class QTemporaryDir;

class QGCTileSetImporterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testImportMBTiles();
    void _testImportMBTilesYFlip();
    void _testImportUnsupportedExtension();
    void _testImportMissingFile();
    void _testPMTilesZxyToTileId();
    void _testImportPMTiles();
    void _testPMTilesDirEntryCountTooLarge();
    void _testPMTilesTileOutsideDataRegion();
    void _testPMTilesLeafCycle();
    void _testPMTilesZoomTooLarge();
    void _testDecompressCapRejectsOversize();
    void _testImportMBTilesDedupSchema();

private:
    std::unique_ptr<QGCTileCacheDatabase> _createInitializedDB(QTemporaryDir &tempDir);
    QString _writeMBTilesFixture(QTemporaryDir &tempDir, const QString &fileName);
    QString _writePMTilesFixture(QTemporaryDir &tempDir, const QString &fileName);
    QString _writeMalformedPMTiles(QTemporaryDir &tempDir, const QString &fileName,
                                   const std::function<void(QByteArray &, QByteArray &, QByteArray &)> &mutate);
};
