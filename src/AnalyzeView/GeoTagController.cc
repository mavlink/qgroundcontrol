/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GeoTagController.h"
#include "QGCFileDialog.h"

GeoTagController::GeoTagController(void)
    : _progress(0)
    , _inProgress(false)
{
    connect(&_worker, &GeoTagWorker::progressChanged,   this, &GeoTagController::_workerProgressChanged);
    connect(&_worker, &GeoTagWorker::error,             this, &GeoTagController::_workerError);
    connect(&_worker, &GeoTagWorker::started,           this, &GeoTagController::inProgressChanged);
    connect(&_worker, &GeoTagWorker::finished,          this, &GeoTagController::inProgressChanged);
}

GeoTagController::~GeoTagController()
{

}

void GeoTagController::pickLogFile(void)
{
    QString filename = QGCFileDialog::getOpenFileName(NULL, "Select log file load", QString(), "PX4 log file (*.px4log);;All Files (*.*)");
    if (!filename.isEmpty()) {
        _worker.setLogFile(filename);
        emit logFileChanged(filename);
    }
}

void GeoTagController::pickImageDirectory(void)
{
    QString dir = QGCFileDialog::getExistingDirectory(NULL, "Select image directory");
    if (!dir.isEmpty()) {
        _worker.setImageDirectory(dir);
        emit imageDirectoryChanged(dir);
    }
}

void GeoTagController::startTagging(void)
{
    _errorMessage.clear();
    emit errorMessageChanged(_errorMessage);
    _worker.start();
}

void GeoTagController::_workerProgressChanged(double progress)
{
    _progress = progress;
    emit progressChanged(progress);
}

void GeoTagController::_workerError(QString errorMessage)
{
    _errorMessage = errorMessage;
    emit errorMessageChanged(errorMessage);
}

GeoTagWorker::GeoTagWorker(void)
    : _cancel(false)
{

}

void GeoTagWorker::run(void)
{
    _cancel = false;
    emit progressChanged(0);

    for (int i=0; i<10;i++) {
        if (_cancel) {
            emit error(tr("Tagging cancelled"));
            return;
        }
        emit progressChanged(i*10);
        sleep(1);
    }

    emit progressChanged(100);
    emit taggingComplete();
}
