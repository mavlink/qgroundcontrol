#ifndef SYSTEMVIEWPARAMS_H
#define SYSTEMVIEWPARAMS_H

#include <QMap>
#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <QWidget>

class SystemViewParams : public QObject
{
    Q_OBJECT

public:
    SystemViewParams(int systemId);

    bool& colorPointCloudByDistance(void);
    bool colorPointCloudByDistance(void) const;

    bool& displayLocalGrid(void);
    bool displayLocalGrid(void) const;

    bool& displayObstacleList(void);
    bool displayObstacleList(void) const;

    QMap<QString,bool>& displayOverlay(void);
    QMap<QString,bool> displayOverlay(void) const;

    bool& displayPlannedPath(void);
    bool displayPlannedPath(void) const;

    bool& displayPointCloud(void);
    bool displayPointCloud(void) const;

    bool& displayRGBD(void);
    bool displayRGBD(void) const;

    bool& displaySetpoints(void);
    bool displaySetpoints(void) const;

    bool& displayTarget(void);
    bool displayTarget(void) const;

    bool& displayTrails(void);
    bool displayTrails(void) const;

    bool& displayWaypoints(void);
    bool displayWaypoints(void) const;

    int& modelIndex(void);
    int modelIndex(void) const;

    QVector<QString>& modelNames(void);
    const QVector<QString>& modelNames(void) const;

    int& setpointHistoryLength(void);
    int setpointHistoryLength(void) const;

public slots:
    void modelChanged(int index);
    void setSetpointHistoryLength(int length);
    void toggleColorPointCloud(int state);
    void toggleLocalGrid(int state);
    void toggleObstacleList(int state);
    void toggleOverlay(QWidget* widget);
    void togglePlannedPath(int state);
    void togglePointCloud(int state);
    void toggleRGBD(int state);
    void toggleSetpoints(int state);
    void toggleTarget(int state);
    void toggleTrails(int state);
    void toggleWaypoints(int state);

signals:
    void modelChangedSignal(int systemId, int index);

private:
    int mSystemId;

    bool mColorPointCloudByDistance;
    bool mDisplayLocalGrid;
    bool mDisplayObstacleList;
    QMap<QString,bool> mDisplayOverlay;
    bool mDisplayPlannedPath;
    bool mDisplayPointCloud;
    bool mDisplayRGBD;
    bool mDisplaySetpoints;
    bool mDisplayTarget;
    bool mDisplayTrails;
    bool mDisplayWaypoints;
    int mModelIndex;
    QVector<QString> mModelNames;
    int mSetpointHistoryLength;
};

typedef QSharedPointer<SystemViewParams> SystemViewParamsPtr;

#endif // SYSTEMVIEWPARAMS
