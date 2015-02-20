#ifndef QGCMAPTOOLBAR_H
#define QGCMAPTOOLBAR_H

#include <QWidget>
#include <QMenu>
#include <QActionGroup>

class QGCMapWidget;

namespace Ui {
    class QGCMapToolBar;
}

class QGCMapToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMapToolBar(QWidget *parent = 0);

    void setMap(QGCMapWidget* map);

public slots:
    void tileLoadStart();
    void tileLoadEnd();
    void tileLoadProgress(int progress);
    void setUAVTrailTime();
    void setUAVTrailDistance();
    void setUpdateInterval();
    void setMapType();
    void setStatusLabelText(const QString &text);

private:
    Ui::QGCMapToolBar* _ui;

    QGCMapWidget* _map;
    QMenu* _optionsMenu;
    QMenu* _trailPlotMenu;
    QMenu* _updateTimesMenu;
    QMenu* _mapTypesMenu;

    QActionGroup* _trailSettingsGroup;
    QActionGroup* _updateTimesGroup;
    QActionGroup* _mapTypesGroup;

    unsigned _statusMaxLen;
};

#endif // QGCMAPTOOLBAR_H
