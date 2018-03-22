#ifndef ENERGYBUDGET_H
#define ENERGYBUDGET_H

#include <QWidget>
#include <stdint.h>
#include "QGCDockWidget.h"
#include "ui_EnergyBudget.h"
#include "Vehicle.h"

#define LEFTBATMONCOMPID 150
#define CENTERBATMONCOMPID 151
#define RIGHTBATMONCOMPID 152


namespace Ui {
class EnergyBudget;
}

class QGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsPathItem;
class QGraphicsTextItem;
class UASInterface;
class QTimer;
class Hysteresisf;
class UASInterface;

class EnergyBudget : public QGCDockWidget
{
    Q_OBJECT

public:
    explicit EnergyBudget(const QString& title, QAction* action, QWidget *parent = 0);
    ~EnergyBudget();

protected:
    Ui::EnergyBudget *ui;
	QGraphicsScene *m_scene;
	QGraphicsPixmapItem *m_propPixmap;
	QGraphicsPixmapItem *m_cellPixmap;
	QGraphicsPixmapItem *m_batPixmap;
	QGraphicsPathItem *m_chargePath;
	QGraphicsPathItem *m_cellToPropPath;
	QGraphicsPathItem *m_batToPropPath;
	QGraphicsTextItem *m_chargePowerText;
	QGraphicsTextItem *m_cellPowerText;
	QGraphicsTextItem *m_BckpBatText;
	QGraphicsTextItem *m_cellUsePowerText;
	QGraphicsTextItem *m_batUsePowerText;
    QGraphicsTextItem *m_SystemPowerText;
	quint64 m_lastBckpBatWarn;
	float m_cellPower;
	float m_batUsePower;
    float m_SystemPower;
	float m_chargePower;
	double m_thrust;

	enum class batChargeStatus : int8_t
	{
		DSCHRG,
		LVL,
		CHRG
	};
	batChargeStatus m_batCharging;
	Hysteresisf *m_batHystHigh;
	Hysteresisf *m_batHystLow;
	Hysteresisf *m_mpptHyst;
	QTimer *m_MPPTUpdateReset;

	void buildGraphicsImage();
    qreal adjustImageScale(const QRectF&, QRectF&);
    //virtual void resizeEvent(QResizeEvent * event);
    void updateGraphicsImage(void);
	QString convertHostfet(uint16_t);
	QString convertBatteryStatus(uint16_t);
	QString convertMPPTModus(uint8_t);
	QString convertMPPTStatus(uint8_t bit);
    QString convertPowerboardStatus(uint8_t statusbit);

protected slots:
	void updateMPPT(float volt1, float amp1, uint16_t pwm1, uint8_t status1, float volt2, float amp2, uint16_t pwm2, uint8_t status2, float volt3, float amp3, uint16_t pwm3, uint8_t status3);
    //void updateMPPTHL(uint8_t volt1, uint8_t volt2, uint8_t volt3);
    void updateBatMon(uint8_t compid, uint16_t volt, int16_t current, uint8_t soc, float temp, uint16_t batStatus, uint32_t batSafety, uint32_t batOperation, uint16_t cellvolt1, uint16_t cellvolt2, uint16_t cellvolt3, uint16_t cellvolt4, uint16_t cellvolt5, uint16_t cellvolt6);

    //void updateBatMonHL(uint8_t compid, uint8_t volt, int8_t p_avg, uint8_t batmonStatusByte);
    void updatePowerBoard(uint64_t timestamp, uint8_t status, uint8_t led_status, float system_volt, float servo_volt, float digital_volt, float mot_l_amp, float mot_r_amp, float analog_amp, float digital_amp, float ext_amp, float aux_amp);
    void setActiveUAS(Vehicle* vehicle);
	void styleChanged(bool);
    void MPPTTimerTimeout(void);
    void onThrustChanged(Vehicle* vehicle, double);

    void ResetMPPTCmd(void);
};

#endif // ENERGYBUDGET_H
