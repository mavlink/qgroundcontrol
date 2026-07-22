#include "FTPManagerJob.h"

#include "FTPManager.h"

FTPJob::FTPJob(FTPManager* manager, Type type) : QObject(manager), _manager(manager), _type(type) {}

FTPJob::~FTPJob()
{
    if (_manager && !_finished) {
        _manager->_cancelJob(this);
    }
}

void FTPJob::cancel()
{
    if (_manager && !_finished) {
        _manager->_cancelJob(this);
    }
}

void FTPJob::_finish()
{
    _finished = true;
    _manager = nullptr;
}

FTPDownloadJob::FTPDownloadJob(FTPManager* manager) : FTPJob(manager, Type::Download) {}

FTPUploadJob::FTPUploadJob(FTPManager* manager) : FTPJob(manager, Type::Upload) {}

FTPListDirectoryJob::FTPListDirectoryJob(FTPManager* manager) : FTPJob(manager, Type::ListDirectory) {}

FTPDeleteJob::FTPDeleteJob(FTPManager* manager) : FTPJob(manager, Type::Delete) {}
