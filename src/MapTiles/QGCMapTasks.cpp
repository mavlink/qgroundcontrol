#include "QGCMapTasks.h"

#include "QGCCacheTile.h"
#include "QGCCachedTileSet.h"
#include "QGCMapUrlEngine.h"

QGCCreateTileSetTask::QGCCreateTileSetTask(QGCCachedTileSet* tileSet, QObject* parent)
    : QGCMapTask(TaskType::taskCreateTileSet, parent)
    , m_tileSet(tileSet)
    , m_cacheVariant(UrlFactory::cacheVariant(tileSet->type()))
    , m_saved(false)
{}

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

QGCImportTileTask::QGCImportTileTask(const QString& path, bool replace, QObject* parent)
    : QGCMapTask(TaskType::taskImport, parent)
    , m_path(path)
    , m_replace(replace)
    , m_cacheVariants(UrlFactory::cacheVariants())
{}
