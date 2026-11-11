#include "QGCMapTasks.h"

#include "QGCCacheTile.h"
#include "QGCCachedTileSet.h"

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
