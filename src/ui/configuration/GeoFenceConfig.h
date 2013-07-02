#ifndef GEOFENCECONFIG_H
#define GEOFENCECONFIG_H

#include <QWidget>
#include "ui_GeoFenceConfig.h"

class GeoFenceConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit GeoFenceConfig(QWidget *parent = 0);
    ~GeoFenceConfig();
    
private:
    Ui::GeoFenceConfig ui;
};

#endif // GEOFENCECONFIG_H
