#include "QGCMapTasks.h"

#include "QGCCachedTileSet.h"
#include "QGCCacheTile.h"

QGCCreateTileSetTask::~QGCCreateTileSetTask()
{
    if (!m_saved) {
        delete m_tileSet;
    }
}

QGCSaveTileTask::~QGCSaveTileTask()
{
    delete m_tile;
}
