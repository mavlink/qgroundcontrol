#pragma once

#include "QmlUITestBase.h"

/// UI test of the editor of an offline map set, which Settings > Maps opens: for a new set, the two previews of the
/// zoom levels of the set are maps of their own beside the map of the editor, and for a set there is, the editor
/// tells of its tiles.
class OfflineMapEditorUITest : public QmlUITestBase
{
    Q_OBJECT

public:
    OfflineMapEditorUITest() = default;

private slots:
    void _testZoomPreviewsShowMapOfEditor();
    void _testNewSetIsOfMapWithServer();
    void _testInfoShowsTilesOfDefaultSet();
    void _testClearCacheDeletesCachedTiles();
};
