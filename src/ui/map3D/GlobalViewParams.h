#ifndef GLOBALVIEWPARAMS_H
#define GLOBALVIEWPARAMS_H

#include <QObject>
#include <QString>
#include <QVector3D>

#include "QGCMAVLink.h"
#include "Imagery.h"

class GlobalViewParams : public QObject
{
    Q_OBJECT

public:
    GlobalViewParams();

    bool& displayTerrain(void);
    bool displayTerrain(void) const;

    bool& displayWorldGrid(void);
    bool displayWorldGrid(void) const;

    QVector3D& imageryOffset(void);
    QVector3D imageryOffset(void) const;

    QString& imageryPath(void);
    QString imageryPath(void) const;

    Imagery::Type& imageryType(void);
    Imagery::Type imageryType(void) const;

    int& followCameraId(void);
    int followCameraId(void) const;

    MAV_FRAME& frame(void);
    MAV_FRAME frame(void) const;

    void signalImageryParamsChanged(void);

    QVector3D& terrainPositionOffset(void);
    QVector3D terrainPositionOffset(void) const;

    QVector3D& terrainAttitudeOffset(void);
    QVector3D terrainAttitudeOffset(void) const;

public slots:
    void followCameraChanged(const QString& text);
    void frameChanged(const QString &text);
    void toggleTerrain(int state);
    void toggleWorldGrid(int state);

signals:
    void followCameraChanged(int systemId);
    void imageryParamsChanged(void);

private:
    bool mDisplayTerrain;
    bool mDisplayWorldGrid;
    QVector3D mImageryOffset;
    QString mImageryPath;
    Imagery::Type mImageryType;
    int mFollowCameraId;
    MAV_FRAME mFrame;
    QVector3D mTerrainPositionOffset;
    QVector3D mTerrainAttitudeOffset;
};

typedef QSharedPointer<GlobalViewParams> GlobalViewParamsPtr;

#endif // GLOBALVIEWPARAMS_H
