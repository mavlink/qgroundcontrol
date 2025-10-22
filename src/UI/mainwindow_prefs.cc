#include "mainwindow_prefs.h"

static inline QSettings& s()
{
    static thread_local QSettings settings; // QGC sets org/app globally
    return settings;
}

MainWindowPrefs::MainWindowPrefs(QObject* parent)
    : QObject(parent)
{
    _load();
}

void MainWindowPrefs::_load()
{
    s().beginGroup("MainWindow");
    _startFullScreen = s().value("startFullScreen", _startFullScreen).toBool();
    _normalX = s().value("normalX", _normalX).toInt();
    _normalY = s().value("normalY", _normalY).toInt();
    _normalW = s().value("normalW", _normalW).toInt();
    _normalH = s().value("normalH", _normalH).toInt();
    s().endGroup();
}

void MainWindowPrefs::_saveOne(const char* key, const QVariant& v) const
{
    s().beginGroup("MainWindow");
    s().setValue(key, v);
    s().endGroup();
}

void MainWindowPrefs::setStartFullScreen(bool v){ if (_startFullScreen==v) return; _startFullScreen=v; _saveOne("startFullScreen", v); emit startFullScreenChanged(); }
void MainWindowPrefs::setNormalX(int v){ if (_normalX==v) return; _normalX=v; _saveOne("normalX", v); emit normalXChanged(); }
void MainWindowPrefs::setNormalY(int v){ if (_normalY==v) return; _normalY=v; _saveOne("normalY", v); emit normalYChanged(); }
void MainWindowPrefs::setNormalW(int v){ if (_normalW==v) return; _normalW=v; _saveOne("normalW", v); emit normalWChanged(); }
void MainWindowPrefs::setNormalH(int v){ if (_normalH==v) return; _normalH=v; _saveOne("normalH", v); emit normalHChanged(); }
