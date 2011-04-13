#ifndef QGCMAPWIDGET_H
#define QGCMAPWIDGET_H

#include "opmapcontrol.h"

class UASInterface;

class QGCMapWidget : public mapcontrol::OPMapWidget
{
    Q_OBJECT
public:
    explicit QGCMapWidget(QWidget *parent = 0);

signals:

public slots:
    void addUAS(UASInterface* uas);
    void updateGlobalPosition(UASInterface* uas, double lat, double lon, double alt, quint64 usec);

};

#endif // QGCMAPWIDGET_H
