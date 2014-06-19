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
    ~QGCMapToolBar();

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
    Ui::QGCMapToolBar *ui;

protected:
    QGCMapWidget* map;
    QMenu optionsMenu;
    QMenu trailPlotMenu;
    QMenu updateTimesMenu;
    QMenu mapTypesMenu;

    QActionGroup* trailSettingsGroup;
    QActionGroup* updateTimesGroup;
    QActionGroup* mapTypesGroup;

    unsigned statusMaxLen;
};

#endif // QGCMAPTOOLBAR_H
