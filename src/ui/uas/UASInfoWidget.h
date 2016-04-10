/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Detail information of one MAV
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _UASINFOWIDGET_H_
#define _UASINFOWIDGET_H_

#include <QTimer>
#include <QMap>

#include "QGCDockWidget.h"
#include "UASInterface.h"
#include "ui_UASInfo.h"
#include "Vehicle.h"

/**
 * @brief Info indicator for the currently active UAS
 *
 **/
class UASInfoWidget : public QGCDockWidget
{
    Q_OBJECT
public:
    UASInfoWidget(const QString& title, QAction* action, QWidget *parent = 0, QString name = "");
    ~UASInfoWidget();

public slots:
    void updateBattery(UASInterface* uas, double voltage, double current, double percent, int seconds);
    void updateCPULoad(UASInterface* uas, double load);
    /**
     * @brief Set the loss rate of packets received by the MAV.
     * @param uasId UNUSED
     * @param receiveLoss A percentage value (0-100) of how many message the UAS has failed to receive.
     */
    void updateReceiveLoss(int uasId, float receiveLoss);

    void updateSeqLossPercent(int uasId, float seqLoss);
    void updateSeqLossTotal(int uasId, int seqLossTotal);

    /**
	 * @brief Set the loss rate of packets sent from the MAV 
	 * @param uasId UNUSED
	 * @param sendLoss A percentage value (0-100) of how many message QGC has failed to receive.
	 */
    void updateSendLoss(int uasId, float sendLoss);
	
    /** @brief Update the error count */
    void updateErrorCount(int uasid, QString component, QString device, int count);

    void setVoltage(UASInterface* uas, double voltage);
    void setChargeLevel(UASInterface* uas, double chargeLevel);
    void setTimeRemaining(UASInterface* uas, double seconds);

    void refresh();

protected:


    // Configuration variables
    int voltageDecimals;
    int loadDecimals;

    // State variables

    // Voltage
    double voltage;
    double chargeLevel;
    double timeRemaining;
    double load;
    float receiveLoss;
    float sendLoss;
    bool changed;
    QTimer* updateTimer;
    QString name;
    quint64 startTime;
    QMap<QString, int> errors;
    static const int updateInterval = 800; ///< Refresh interval in milliseconds

    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    
private slots:
    void _activeVehicleChanged(Vehicle* vehicle);

private:
    Ui::uasInfo ui;

    UASInterface*   _activeUAS;
    float           _seqLossPercent;
    int             _seqLossTotal;
};

#endif // _UASINFOWIDGET_H_
