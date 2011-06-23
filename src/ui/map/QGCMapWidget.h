#ifndef QGCMAPWIDGET_H
#define QGCMAPWIDGET_H

#include "opmapcontrol.h"

class UASInterface;

class QGCMapWidget : public mapcontrol::OPMapWidget
{
    Q_OBJECT
public:
    explicit QGCMapWidget(QWidget *parent = 0);
    ~QGCMapWidget();

signals:

public slots:
    void addUAS(UASInterface* uas);
    void updateGlobalPosition(UASInterface* uas, double lat, double lon, double alt, quint64 usec);
    /** @brief Update the type, size, etc. of this system */
    void updateSystemSpecs(int uas);
    /** @brief Update the whole system state */
    void updateSelectedSystem(int uas);

protected:
    /** @brief Update the attitude of this system */
    void updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 usec);

};

#endif // QGCMAPWIDGET_H
