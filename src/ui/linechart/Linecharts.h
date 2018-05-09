#ifndef LINECHARTS_H
#define LINECHARTS_H

#include <QStackedWidget>
#include <QMap>
#include <QVector>

#include "LinechartWidget.h"
#include "Vehicle.h"
#include "MultiVehicleDockWidget.h"
#include "MAVLinkDecoder.h"

class Linecharts : public MultiVehicleDockWidget
{
    Q_OBJECT
public:
    explicit Linecharts(const QString& title, QAction* action, MAVLinkDecoder* decoder, QWidget *parent = 0);

signals:
    /** @brief This signal is emitted once a logfile has been finished writing */
    void logfileWritten(QString fileName);
    void visibilityChanged(bool visible);

protected:
    // Override from MultiVehicleDockWidget
    virtual QWidget* _newVehicleWidget(Vehicle* vehicle, QWidget* parent);

private:
    MAVLinkDecoder* _mavlinkDecoder;
};

#endif // LINECHARTS_H
