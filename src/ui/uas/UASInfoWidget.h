/*=====================================================================
 
PIXHAWK Micro Air Vehicle Flying Robotics Toolkit
 
(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>
 
This file is part of the PIXHAWK project
 
    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
 
    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.
 
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

#include <QWidget>
#include <QTimer>
#include <QMap>

#include "UASInterface.h"
#include "ui_UASInfo.h"

/**
 * @brief Info indicator for the currently active UAS
 *
 **/
class UASInfoWidget : public QWidget {
    Q_OBJECT
public:
    UASInfoWidget(QWidget *parent = 0, QString name = "");
    ~UASInfoWidget();

public slots:
    void addUAS(UASInterface* uas);

    void setActiveUAS(UASInterface* uas);

    void updateBattery(UASInterface* uas, double voltage, double percent, int seconds);
    void updateCPULoad(UASInterface* uas, double load);
    void updateReceiveLoss(float receiveLoss);
    void updateDropRate(int sysId, float receiveDrop, float sendDrop);

    void setVoltage(UASInterface* uas, double voltage);
    void setChargeLevel(UASInterface* uas, double chargeLevel);
    void setTimeRemaining(UASInterface* uas, double seconds);
//    void setBattery(int uasid, BatteryType type, int cells);

//    void valueChanged(int uasid, QString key, double value,quint64 time);
//    void actuatorChanged(UASInterface* uas, int actId, double value);
    void refresh();

protected:

    UASInterface* activeUAS;

    // Configuration variables
    int voltageDecimals;
    int loadDecimals;

    // State variables

    // Voltage
    double voltage;
    double chargeLevel;
    double timeRemaining;
    double load;
    QTimer* updateTimer;
    QString name;
    quint64 startTime;
//    double lastRemainingTime;
//    double lastChargeLevel;
//    double startVoltage;
//    double fullVoltage;
//    double emptyVoltage;
//    double currentVoltage;
    // Battery Type
//    BatteryType batteryType;
    // Number of cells
//    int cells;

    /*
    QMap<QString, QProgressBar*>* instruments;

    class Instrument
    {
        public:
        void setMin(double min)
        {
            this->min = min;
        }

                void setMax(double max)
        {
            this->max = max;
        }

                        void setValue(double value)
        {
            this->value = value;
        }

        void refresh()
        {
            bar.setValue(value);
        }


        protected:
            double min;
            double max;
            double value;
            QString unit;
            QLabel label;
            QProgressBar bar
    };*/

private:
    Ui::uasInfo ui;

};

#endif // _UASINFOWIDGET_H_
