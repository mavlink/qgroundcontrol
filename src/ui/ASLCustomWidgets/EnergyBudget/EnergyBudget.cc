#include "EnergyBudget.h"
#include "ui_EnergyBudget.h"
#include <qgraphicsscene.h>
#include <QGraphicsPixmapItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QMessageBox>
#include <QTimer>
#include <math.h>
#include <qdebug.h>
#include "UASInterface.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

#define OVERVIEWTOIMAGEHEIGHTSCALE (3.0)
#define OVERVIEWTOIMAGEWIDTHSCALE (3.0)

#define CELLPOWERHYSTMIN 1
#define CELLPOWERHYSTMAX 3
#define BATPOWERHIGHMIN 1
#define BATPOWERHIGHMAX 3
#define BATPOWERLOWMIN -3
#define BATPOWERLOWMAX -1
#define MPPTRESETTIMEMS 3000

class Hysteresisf
{
public:
	Hysteresisf(float minVal, float maxVal, bool isHighState);
	bool check(float);
private:
	float const m_minVal;
	float const m_maxVal;
    bool m_highState;
};

EnergyBudget::EnergyBudget(const QString &title, QAction *action, QWidget *parent) :
QGCDockWidget(title, action, parent),
ui(new Ui::EnergyBudget),
m_scene(new QGraphicsScene(this)),
m_propPixmap(m_scene->addPixmap(QPixmap(":/qmlimages/EnergyBudget/Plane"))),
m_cellPixmap(m_scene->addPixmap(QPixmap(":/qmlimages/EnergyBudget/Solarcell"))),
m_batPixmap(m_scene->addPixmap(QPixmap(":/qmlimages/EnergyBudget/Battery"))),
m_chargePath(m_scene->addPath(QPainterPath())),
m_cellToPropPath(m_scene->addPath(QPainterPath())),
m_batToPropPath(m_scene->addPath(QPainterPath())),
m_chargePowerText(m_scene->addText(QString("N/A"))),
m_cellPowerText(m_scene->addText(QString("N/A"))),
m_BckpBatText(m_scene->addText(QString("Critical: On backup battery!!!"))),
m_cellUsePowerText(m_scene->addText(QString("N/A"))),
m_batUsePowerText(m_scene->addText(QString("N/A"))),
m_SystemPowerText(m_scene->addText(QString("N/A"))),
m_lastBckpBatWarn(0),
m_cellPower(0.0),
m_batUsePower(0.0),
m_SystemPower(0.0),
m_chargePower(0.0),
m_thrust(0.0),
m_batCharging(batChargeStatus::CHRG),
m_batHystHigh(new Hysteresisf(BATPOWERHIGHMIN, BATPOWERHIGHMAX, true)),
m_batHystLow(new Hysteresisf(BATPOWERLOWMIN, BATPOWERLOWMAX, true)),
m_mpptHyst(new Hysteresisf(CELLPOWERHYSTMIN, CELLPOWERHYSTMAX, true)),
m_MPPTUpdateReset(new QTimer(this))
{
    ui->setupUi(this);
    ui->overviewGraphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->overviewGraphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	this->buildGraphicsImage();
    ui->overviewGraphicsView->setScene(m_scene);
    ui->overviewGraphicsView->fitInView(QRectF(0,0,150,100), Qt::AspectRatioMode::KeepAspectRatio);
    //ui->overviewGraphicsView->fitInView(m_scene->sceneRect(), Qt::AspectRatioMode::KeepAspectRatio);
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleAdded, this, &EnergyBudget::setActiveUAS);

	connect(ui->ResetMPPTButton, SIGNAL(clicked()), this, SLOT(ResetMPPTCmd()));
    //connect(qgcApp(), SIGNAL(styleChanged(bool)), this, SLOT(styleChanged(bool)));
	m_MPPTUpdateReset->setInterval(MPPTRESETTIMEMS);
	m_MPPTUpdateReset->setSingleShot(false);
	connect(m_MPPTUpdateReset, SIGNAL(timeout()), this, SLOT(MPPTTimerTimeout()));
    if (qgcApp()->toolbox()->multiVehicleManager()->activeVehicle())
    {
        setActiveUAS(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle());
    }
    m_MPPTUpdateReset->start();
}

EnergyBudget::~EnergyBudget()
{
    delete ui;
    delete m_scene;
    delete m_MPPTUpdateReset;
    delete m_batHystHigh;
    delete m_batHystLow;
    delete m_mpptHyst;
}

void EnergyBudget::buildGraphicsImage()
{
    m_scene->setSceneRect(0.0, 0.0, 1000.0, 666.0);
    qreal penWidth(40);

    // Scale and place images
    QRectF tempRectF=m_propPixmap->boundingRect();
    m_propPixmap->setScale(2.0*this->adjustImageScale(m_scene->sceneRect(), tempRectF ));
    tempRectF=m_cellPixmap->boundingRect();
    m_cellPixmap->setScale(this->adjustImageScale(m_scene->sceneRect(), tempRectF ));
    tempRectF=m_batPixmap->boundingRect();
    m_batPixmap->setScale(this->adjustImageScale(m_scene->sceneRect(), tempRectF ));
	double maxheight(std::fmax(m_propPixmap->mapRectToScene(m_propPixmap->boundingRect()).height(), m_batPixmap->mapRectToScene(m_batPixmap->boundingRect()).height()));
    m_propPixmap->setOffset(m_propPixmap->mapRectFromScene(m_scene->sceneRect()).width()/2.0 - m_propPixmap->boundingRect().width()/2.0, m_propPixmap->mapRectFromScene(m_scene->sceneRect()).height() - m_propPixmap->mapRectFromScene(QRectF(0.0,0.0,1.0,maxheight)).height()/2.0 - m_propPixmap->boundingRect().height() / 2.0);
    m_batPixmap->setOffset(m_batPixmap->mapRectFromScene(m_scene->sceneRect()).width() - m_batPixmap->boundingRect().width(), 0.0);//m_batPixmap->mapRectFromScene(m_scene->sceneRect()).height() - m_batPixmap->mapRectFromScene(QRectF(0.0, 0.0, 1.0, maxheight)).height() / 2.0 - m_batPixmap->boundingRect().height() / 2.0);
    m_cellPixmap->setOffset(0.0,0.0);//m_cellPixmap->mapRectFromScene(m_scene->sceneRect()).width() / 2.0, 0.0);

    // Add arrows between images
	QRectF cellRect(m_cellPixmap->sceneBoundingRect());
	QRectF propRect(m_propPixmap->sceneBoundingRect());
	QRectF batRect(m_batPixmap->sceneBoundingRect());
    QPainterPath cellPath(QPointF(cellRect.x() + cellRect.width() + penWidth, cellRect.y() + cellRect.height() / 2.0));
    cellPath.lineTo(batRect.x()-penWidth, cellRect.y() + cellRect.height() / 2.0);
    m_chargePath->setPen(QPen(QBrush(Qt::GlobalColor::green, Qt::BrushStyle::SolidPattern), penWidth));
    m_chargePath->setPath(m_chargePath->mapFromScene(cellPath));
    QPainterPath cell2PropPath(QPointF(cellRect.x()+ cellRect.width()/2.0, cellRect.y() + cellRect.height() + penWidth));
    cell2PropPath.lineTo(cellRect.x()+ cellRect.width()/2.0, propRect.y() + propRect.height() / 2.0);
    cell2PropPath.lineTo(propRect.x()-0.5*penWidth, propRect.y() + propRect.height() / 2.0);
    m_cellToPropPath->setPen(QPen(QBrush(Qt::GlobalColor::green, Qt::BrushStyle::SolidPattern), penWidth));
    m_cellToPropPath->setPath(m_cellToPropPath->mapFromScene(cell2PropPath));
    QPainterPath bat2PropPath(QPointF(batRect.x()+batRect.width()/2.0, batRect.y() + batRect.height() + penWidth));
    bat2PropPath.lineTo(batRect.x()+batRect.width()/2.0,propRect.y() + propRect.height()/2.0);
    bat2PropPath.lineTo(propRect.x()+propRect.width() + 0.5*penWidth, propRect.y() + propRect.height()/2.0);
    m_batToPropPath->setPen(QPen(QBrush(Qt::GlobalColor::red, Qt::BrushStyle::SolidPattern), penWidth));
    m_batToPropPath->setPath(m_batToPropPath->mapFromScene(bat2PropPath));

    // Add text
    m_chargePowerText->setPos((propRect.x() + batRect.x()) / 2.0 - 1*penWidth, (cellRect.y()+cellRect.height())/ 2.0+0.8*penWidth);
	m_chargePowerText->setFont(QFont("Helvetica", penWidth*1.2));
    m_batUsePowerText->setPos(batRect.x() + batRect.width()/2.0 - 5.5*penWidth, batRect.y() + batRect.height() + 2*penWidth);
	m_batUsePowerText->setFont(QFont("Helvetica", penWidth*1.2));
    m_cellPowerText->setPos(cellRect.x(), cellRect.y() - 2*penWidth);
	m_cellPowerText->setFont(QFont("Helvetica", penWidth*1.2));
    m_cellUsePowerText->setPos(propRect.x() + 0.2*penWidth, cellRect.y() + cellRect.height() + 2*penWidth);
	m_cellUsePowerText->setFont(QFont("Helvetica", penWidth*1.2));
    m_SystemPowerText->setPos(propRect.x() + 2*penWidth, propRect.y() + propRect.height());
    m_SystemPowerText->setFont(QFont("Helvetica", penWidth*1.2));
    m_BckpBatText->setPos(propRect.x()-1.5*penWidth, propRect.y()-2*penWidth);
    m_BckpBatText->setFont(QFont("Helvetica", penWidth*1.2));
    m_BckpBatText->setVisible(false);

    // set the right textcolor
    styleChanged(qgcApp()->toolbox()->settingsManager()->appSettings()->indoorPalette()->rawValue().toBool());

	// Recalc scene bounding rect
    m_scene->setSceneRect(QRectF(0.0, 0.0, 0.0, 0.0));
}

qreal EnergyBudget::adjustImageScale(const QRectF &viewSize, QRectF &img)
{
	return std::fmin((viewSize.height() / (OVERVIEWTOIMAGEHEIGHTSCALE*img.height())), (viewSize.width() / (OVERVIEWTOIMAGEWIDTHSCALE*img.width())));
}

void EnergyBudget::updatePowerBoard(uint64_t timestamp, uint8_t status, uint8_t led_status, float system_volt, float servo_volt, float digital_volt, float mot_l_amp, float mot_r_amp, float analog_amp, float digital_amp, float ext_amp, float aux_amp)
{
#define BCKPBATREG 0x20
    if (status & BCKPBATREG) {
        m_scene->setBackgroundBrush(Qt::red);
        m_BckpBatText->setVisible(true);
        if ((QGC::groundTimeUsecs() - m_lastBckpBatWarn) > 10000000)
        {
            m_lastBckpBatWarn = QGC::groundTimeUsecs();
            qgcApp()->toolbox()->audioOutput()->say("Critical, System switched to backup battery!");
        }
    }
    else {
        m_scene->setBackgroundBrush(Qt::NoBrush);
        m_BckpBatText->setVisible(false);
    }

    m_SystemPower = mot_l_amp *system_volt + analog_amp * servo_volt + digital_amp * digital_volt + ext_amp * system_volt + aux_amp * digital_volt;
    float system_current = system_volt>0 ? m_SystemPower/system_volt:0;
    m_SystemPowerText->setPlainText(QString("%1W @Thr %4%").arg(m_SystemPower, 0, 'f', 1).arg(m_thrust*100.0,0,'f',0));

    ui->PwrBrdStatusFlags->setText(convertPowerboardStatus(status));
    ui->PwrBrdMotorCurrent->setText(QString("%1A").arg(mot_l_amp, 0, 'f', 1));
    ui->PwrBrdAnalogCurrent->setText(QString("%1A").arg(analog_amp, 0, 'f', 1));
    ui->PwrBrdAuxCurrent->setText(QString("%1A").arg(aux_amp, 0, 'f', 1));
    ui->PwrBrdDigitalCurrent->setText(QString("%1A").arg(digital_amp, 0, 'f', 1));
    ui->PwrBrdExtCurrent->setText(QString("%1A").arg(ext_amp, 0, 'f', 1));

    ui->PwrBrdDigitalVolt->setText(QString("%1V").arg(digital_volt, 0, 'f', 1));
    ui->PwrBrdServoVolt->setText(QString("%1V").arg(servo_volt, 0, 'f', 1));
    ui->PwrBrdSysCurrent->setText(QString("%1A").arg(system_current, 0, 'f', 1));
    ui->PwrBrdSysVolt->setText(QString("%1V").arg(system_volt, 0, 'f', 1));
    ui->PwrBrdSysPower->setText(QString("%1W").arg(m_SystemPower, 0, 'f', 1));

    ui->PwrBrdHealthLED->setColor(QColor(Qt::green));
    ui->PwrBrdStatusFlags->setStyleSheet("QLabel { background-color : green;}");
    if(convertPowerboardStatus(status)!="OK") {
        ui->PwrBrdHealthLED->setColor(QColor(Qt::red));
        ui->PwrBrdStatusFlags->setStyleSheet("QLabel { background-color : red;}");
    }
}

void EnergyBudget::updateBatMon(uint8_t compid, uint16_t volt, int16_t current, uint8_t soc, float temp, uint16_t batStatus, uint32_t batSafety, uint32_t batOperation, uint16_t cellvolt1, uint16_t cellvolt2, uint16_t cellvolt3, uint16_t cellvolt4, uint16_t cellvolt5, uint16_t cellvolt6)
{
    ui->bat1VLabel->setText(QString("%1V").arg(volt/1000.0, 0, 'f', 1));
    ui->bat1ALabel->setText(QString("%1A").arg(current / 1000.0, 0, 'f', 1));
    ui->bat1PowerLabel->setText(QString("%1W").arg((volt / 1000.0)*(current / 1000.0), 0, 'f', 1));
    ui->bat1TempLabel->setText(QString("%1C").arg((temp), 0, 'f', 1));
    ui->bat1SoCBar->setValue(soc);
    QString str="%p% ("+QString::number(volt/1000.0,'f',1)+"V)";
    ui->bat1SoCBar->setFormat(str);
    ui->bat1Cell1Label->setText(QString("%1V").arg(cellvolt1/ 1000.0, 0, 'f', 3));
    ui->bat1Cell2Label->setText(QString("%1V").arg(cellvolt2/ 1000.0, 0, 'f', 3));
    ui->bat1Cell3Label->setText(QString("%1V").arg(cellvolt3/ 1000.0, 0, 'f', 3));
    ui->bat1Cell4Label->setText(QString("%1V").arg(cellvolt4/ 1000.0, 0, 'f', 3));
    ui->bat1Cell5Label->setText(QString("%1V").arg(cellvolt5/ 1000.0, 0, 'f', 3));
    ui->bat1Cell6Label->setText(QString("%1V").arg(cellvolt6/ 1000.0, 0, 'f', 3));

    // TODO: add bit mapping as on pixhawk, such that we emit signal with same stuff that is sent via asl_high_latency
    // Note (PhilippOe): This is a tedious way of status bit interpretation that could be simplified a lot,
    // but was kept for historical reasons for now. Amir can change this.
    uint8_t batmonStatusByte = 0;
    batmonStatusByte |= ((0x1 & (batStatus >> 5)) << 0); 		// FC: fully charged
    batmonStatusByte |= ((0x1 & (batStatus >> 4)) << 1); 		// FD: fully discharged
    batmonStatusByte |= ((0x1 & (batSafety >> 1)) << 2); 		// COV: cell overvoltage
    batmonStatusByte |= ((0x1 & (batSafety >> 0)) << 3); 		// CUV: cell undervoltage
    batmonStatusByte |= ((0x1 & (batSafety >> 8)) << 4); 		// OTC: cell overtemperature during charge
    batmonStatusByte |= ((0x1 & (batOperation >> 2)) << 5); 	// DSG: discharge FET status
    batmonStatusByte |= ((0x1 & (batOperation >> 1)) << 6); 	// CHG: charge FET status
    batmonStatusByte |= ((0x1 & (batOperation >> 12)) << 7); 	// PF: permanent failure

    int failure = 0;
    QString stateFlags="";
    if(0x1 & (batmonStatusByte >> 0)) { stateFlags+="Fully charged \n";}
    if(0x1 & (batmonStatusByte >> 1)) { stateFlags+="Fully discharged \n"; failure=1;}
    if((0x1 & (batmonStatusByte >> 5)) == 0) { stateFlags+="Discharge not allowed. \n"; }
    if(0x1 & (batmonStatusByte >> 2)) { stateFlags+="Cell overvoltage \n"; failure=2;}
    if(0x1 & (batmonStatusByte >> 3)) { stateFlags+="Cell undervoltage \n"; failure=2;}
    if(0x1 & (batmonStatusByte >> 4)) { stateFlags+="Cell overtemperature \n"; failure=2;}
    if((0x1 & (batmonStatusByte >> 6)) == 0) { stateFlags+="Charge not allowed. \n"; }
    if(0x1 & (batmonStatusByte >> 7)) { stateFlags+="Permanent Failure \n"; failure=2;}
    if(stateFlags=="") stateFlags="OK";
    ui->bat1StatusFlags->setText(stateFlags);

    ui->BatteryHealthLED->setColor(QColor(Qt::green));
    ui->bat1StatusFlags->setStyleSheet("QLabel { background-color : green;}");
    if(failure==1) {
        ui->BatteryHealthLED->setColor(QColor(239, 163, 0));
        ui->bat1StatusFlags->setStyleSheet("QLabel { background-color : rgb(239, 163, 0);}");
    } else if (failure==2) {
        ui->BatteryHealthLED->setColor(QColor(Qt::red));
        ui->bat1StatusFlags->setStyleSheet("QLabel { background-color : red;}");
    }

    float power(ui->bat1PowerLabel->text().toDouble());
	if (m_batHystHigh->check(power))
    {
		m_batCharging = batChargeStatus::CHRG;
		m_chargePower = power;
		m_batUsePower = 0;
	}
	else if(!(m_batHystLow->check(power)))
	{
		m_batCharging = batChargeStatus::DSCHRG;
		m_chargePower = 0;
		m_batUsePower = -power;
	}
	else
	{
		m_batCharging = batChargeStatus::LVL;
		m_chargePower = 0;
		m_batUsePower = 0;
	}
	updateGraphicsImage();
}

//void EnergyBudget::updateBatMonHL(uint8_t compid, uint8_t volt, int8_t p_avg, uint8_t batmonStatusByte)
//{
//    switch (compid)
//    {
//    case LEFTBATMONCOMPID:
//            ui->bat1VLabel->setText(QString("%1").arg(volt / 10.0));
//            ui->bat1VLabel_2->setText(QString("%1").arg(volt / 10.0));
//            ui->bat1ALabel->setText(QString("N/A"));
//            ui->bat1PowerLabel->setText(QString("%1").arg(p_avg * 4 / 3.0));
//            ui->bat1CurrentLabel_2->setText(QString("N/A"));
//            ui->bat1StatusFlags->setText(QString("N/A"));
//            ui->bat1SafetyLabel->setText(QString("N/A"));
//            ui->bat1OperLabel->setText(QString("N/A"));
//            ui->bat1TempLabel->setText(QString("N/A"));
//            ui->bat1SoCBar->setValue(QString("N/A").toInt());
//            ui->bat1Cell1Label->setText(QString("N/A"));
//            ui->bat1Cell2Label->setText(QString("N/A"));
//            ui->bat1Cell3Label->setText(QString("N/A"));
//            ui->bat1Cell4Label->setText(QString("N/A"));
//            ui->bat1Cell5Label->setText(QString("N/A"));
//            ui->bat1Cell6Label->setText(QString("N/A"));
//            ui->bat1FCLed->setState((bool)(0x1 & (batmonStatusByte >> 0)));
//            ui->bat1FDLed->setState((bool)(0x1 & (batmonStatusByte >> 1)));
//            ui->bat1COVLed->setState((bool)(0x1 & (batmonStatusByte >> 2)));
//            ui->bat1CUVLed->setState((bool)(0x1 & (batmonStatusByte >> 3)));
//            ui->bat1OTCLed->setState((bool)(0x1 & (batmonStatusByte >> 4)));
//            ui->bat1DSGLed->setState((bool)(0x1 & (batmonStatusByte >> 5)));
//            ui->bat1CHGLed->setState((bool)(0x1 & (batmonStatusByte >> 6)));
//            ui->bat1PFLed->setFlashing((bool)(0x1 & (batmonStatusByte >> 7)));
//            break;

//    case CENTERBATMONCOMPID:
//            ui->bat2VLabel->setText(QString("%1").arg(volt / 10.0));
//            ui->bat2VLabel_2->setText(QString("%1").arg(volt / 10.0));
//            ui->bat2ALabel->setText(QString("N/A"));
//            ui->bat2PowerLabel->setText(QString("%1").arg(p_avg * 4 / 3.0));
//            ui->bat2CurrentLabel_2->setText(QString("N/A"));
//            ui->bat2StatLabel->setText(QString("N/A"));
//            ui->bat2SafetyLabel->setText(QString("N/A"));
//            ui->bat2OperLabel->setText(QString("N/A"));
//            ui->bat2TempLabel->setText(QString("N/A"));
//            ui->bat2SoCBar->setValue(QString("N/A").toInt());
//            ui->bat2Cell1Label->setText(QString("N/A"));
//            ui->bat2Cell2Label->setText(QString("N/A"));
//            ui->bat2Cell3Label->setText(QString("N/A"));
//            ui->bat2Cell4Label->setText(QString("N/A"));
//            ui->bat2Cell5Label->setText(QString("N/A"));
//            ui->bat2Cell6Label->setText(QString("N/A"));
//            ui->bat2FCLed->setState((bool)(0x1 & (batmonStatusByte >> 0)));
//            ui->bat2FDLed->setState((bool)(0x1 & (batmonStatusByte >> 1)));
//            ui->bat2COVLed->setState((bool)(0x1 & (batmonStatusByte >> 2)));
//            ui->bat2CUVLed->setState((bool)(0x1 & (batmonStatusByte >> 3)));
//            ui->bat2OTCLed->setState((bool)(0x1 & (batmonStatusByte >> 4)));
//            ui->bat2DSGLed->setState((bool)(0x1 & (batmonStatusByte >> 5)));
//            ui->bat2CHGLed->setState((bool)(0x1 & (batmonStatusByte >> 6)));
//            ui->bat2PFLed->setFlashing((bool)(0x1 & (batmonStatusByte >> 7)));
//            break;

//    case RIGHTBATMONCOMPID:
//            ui->bat3VLabel->setText(QString("%1").arg(volt / 10.0));
//            ui->bat3VLabel_2->setText(QString("%1").arg(volt / 10.0));
//            ui->bat3ALabel->setText(QString("N/A"));
//            ui->bat3PowerLabel->setText(QString("%1").arg(p_avg * 4 / 3.0));
//            ui->bat3CurrentLabel_2->setText(QString("N/A"));
//            ui->bat3StatLabel->setText(QString("N/A"));
//            ui->bat3SafetyLabel->setText(QString("N/A"));
//            ui->bat3OperLabel->setText(QString("N/A"));
//            ui->bat3TempLabel->setText(QString("N/A"));
//            ui->bat3SoCBar->setValue(QString("N/A").toInt());
//            ui->bat3Cell1Label->setText(QString("N/A"));
//            ui->bat3Cell2Label->setText(QString("N/A"));
//            ui->bat3Cell3Label->setText(QString("N/A"));
//            ui->bat3Cell4Label->setText(QString("N/A"));
//            ui->bat3Cell5Label->setText(QString("N/A"));
//            ui->bat3Cell6Label->setText(QString("N/A"));
//            ui->bat3FCLed->setState((bool)(0x1 & (batmonStatusByte >> 0)));
//            ui->bat3FDLed->setState((bool)(0x1 & (batmonStatusByte >> 1)));
//            ui->bat3COVLed->setState((bool)(0x1 & (batmonStatusByte >> 2)));
//            ui->bat3CUVLed->setState((bool)(0x1 & (batmonStatusByte >> 3)));
//            ui->bat3OTCLed->setState((bool)(0x1 & (batmonStatusByte >> 4)));
//            ui->bat3DSGLed->setState((bool)(0x1 & (batmonStatusByte >> 5)));
//            ui->bat3CHGLed->setState((bool)(0x1 & (batmonStatusByte >> 6)));
//            ui->bat3PFLed->setFlashing((bool)(0x1 & (batmonStatusByte >> 7)));
//            break;
//    }

//    float power((ui->bat1PowerLabel->text().toDouble() + ui->bat2PowerLabel->text().toDouble() + ui->bat3PowerLabel->text().toDouble()));
//    if (m_batHystHigh->check(power))
//    {
//        m_batCharging = batChargeStatus::CHRG;
//        m_chargePower = power;
//        m_batUsePower = 0;
//    }
//    else if(!(m_batHystLow->check(power)))
//    {
//        m_batCharging = batChargeStatus::DSCHRG;
//        m_chargePower = 0;
//        m_batUsePower = -power;
//    }
//    else
//    {
//        m_batCharging = batChargeStatus::LVL;
//        m_chargePower = 0;
//        m_batUsePower = 0;
//    }
//    updateGraphicsImage();
//}



void EnergyBudget::updateMPPT(float volt1, float amp1, uint16_t pwm1, uint8_t status1, float volt2, float amp2, uint16_t pwm2, uint8_t status2, float volt3, float amp3, uint16_t pwm3, uint8_t status3)
{
	m_MPPTUpdateReset->stop();
    ui->mppt1VLabel->setText(QString("%1V").arg(volt1, 0, 'f', 1));
    ui->mppt1ALabel->setText(QString("%1A").arg(amp1, 0, 'f', 1));
    ui->mppt1PowerLabel->setText(QString("%1W").arg(volt1*amp1, 0, 'f', 1));
	ui->mppt1ModLabel->setText(convertMPPTModus(status1));
    ui->mppt1StatLabel->setText(convertMPPTStatus(status1));
    ui->mppt1PwmLabel->setText(QString("%1").arg(pwm1));

    ui->MPPTHealthLED->setColor(QColor(Qt::green));
    ui->mppt1StatLabel->setStyleSheet("QLabel { background-color : green;}");
    if(convertMPPTStatus(status1)!="OK") {
        ui->MPPTHealthLED->setColor(QColor(Qt::red));
        ui->mppt1StatLabel->setStyleSheet("QLabel { background-color : red;}");
    }

    m_cellPower = volt1*amp1;
	updateGraphicsImage();
	m_MPPTUpdateReset->start(MPPTRESETTIMEMS);
}

//void EnergyBudget::updateMPPTHL(uint8_t volt1, uint8_t volt2, uint8_t volt3)
//{
//    m_MPPTUpdateReset->stop();
//    ui->mppt1VLabel->setText(QString("%1").arg(volt1 / 10.0));
//    ui->mppt2VLabel->setText(QString("%1").arg(volt2 / 10.0));
//    ui->mppt3VLabel->setText(QString("%1").arg(volt3 / 10.0));
//    ui->mppt1ALabel->setText(QString("N/A"));
//    ui->mppt2ALabel->setText(QString("N/A"));
//    ui->mppt3ALabel->setText(QString("N/A"));
//    ui->mppt1PowerLabel->setText(QString("N/A"));
//    ui->mppt2PowerLabel->setText(QString("N/A"));
//    ui->mppt3PowerLabel->setText(QString("N/A"));
//    ui->mppt1ModLabel->setText(QString("N/A"));
//    ui->mppt2ModLabel->setText(QString("N/A"));
//    ui->mppt3ModLabel->setText(QString("N/A"));
//    ui->mppt1StatLabel->setText(QString("N/A"));
//    ui->mppt2StatLabel->setText(QString("N/A"));
//    ui->mppt3StatLabel->setText(QString("N/A"));
//    ui->mppt1PwmLabel->setText(QString("N/A"));
//    ui->mppt2PwmLabel->setText(QString("N/A"));
//    ui->mppt3PwmLabel->setText(QString("N/A"));

//    m_cellPower = (QString("N/A").toFloat());
//    updateGraphicsImage();
//    m_MPPTUpdateReset->start(MPPTRESETTIMEMS);
//}

void EnergyBudget::updateGraphicsImage(void)
{
	if (m_batCharging == batChargeStatus::CHRG)
	{
		m_chargePath->setVisible(true);
		m_chargePowerText->setVisible(true);
		m_chargePowerText->setPlainText(QString("%1W").arg(m_chargePower, 0, 'f', 1));
		m_batToPropPath->setVisible(false);
		m_batUsePowerText->setVisible(false);
	}
	else if (m_batCharging == batChargeStatus::DSCHRG)
	{
		m_chargePath->setVisible(false);
		m_chargePowerText->setVisible(false);
		m_batToPropPath->setVisible(true);
		m_batUsePowerText->setVisible(true);
		m_batUsePowerText->setPlainText(QString("%1W").arg(m_batUsePower, 0, 'f', 1));
	}
	else
	{
		m_chargePath->setVisible(false);
		m_chargePowerText->setVisible(false);
		m_batToPropPath->setVisible(false);
		m_batUsePowerText->setVisible(false);
	}
	if (m_mpptHyst->check(m_cellPower))
	{
		m_cellToPropPath->setVisible(true);
		m_cellPowerText->setPlainText(QString("%1W").arg(m_cellPower, 0, 'f', 1));
		m_cellUsePowerText->setVisible(true);
        m_cellUsePowerText->setPlainText(QString("%1W").arg(m_SystemPower - m_batUsePower, 0, 'f', 1));
	}
	else
	{
		m_cellToPropPath->setVisible(false);
		m_cellUsePowerText->setVisible(false);
		m_cellPowerText->setPlainText(QString("0W"));
	}

    // for testing only
    m_chargePath->setVisible(true);
    m_chargePowerText->setVisible(true);
    m_chargePowerText->setPlainText(QString("%1W").arg(m_chargePower, 0, 'f', 1));
    m_batToPropPath->setVisible(true);
    m_batUsePowerText->setVisible(true);
    m_batUsePowerText->setPlainText(QString("%1W").arg(m_batUsePower, 0, 'f', 1));
    m_cellToPropPath->setVisible(true);
    m_cellPowerText->setPlainText(QString("%1W").arg(m_cellPower, 0, 'f', 1));
    m_cellUsePowerText->setVisible(true);
    m_cellUsePowerText->setPlainText(QString("%1W").arg(m_SystemPower - m_batUsePower, 0, 'f', 1));

}

//void EnergyBudget::resizeEvent(QResizeEvent *event)
//{
//    QWidget::resizeEvent(event);
//    ui->overviewGraphicsView->fitInView(m_scene->sceneRect(), Qt::AspectRatioMode::KeepAspectRatio);
//}

void EnergyBudget::setActiveUAS(Vehicle* vehicle)
{
    //disconnect any previous uas
    disconnect(this, SLOT(updateMPPT(float, float, uint16_t, uint8_t, float, float, uint16_t, uint8_t, float, float, uint16_t, uint8_t)));
    //disconnect(this, SLOT(updateMPPTHL(uint8_t, uint8_t, uint8_t)));
    disconnect(this, SLOT(updateBatMon(uint8_t, uint16_t, int16_t, uint8_t, float, uint16_t, uint32_t, uint32_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t)));
    //disconnect(this, SLOT(updateBatMonHL(uint8_t, uint8_t, int8_t, uint8_t)));
    disconnect(this, SLOT(onThrustChanged(Vehicle*,double)));
    disconnect(this, SLOT(updatePowerBoard(uint64_t, uint8_t, uint8_t, float, float, float, float, float, float, float, float, float)));

    //connect the uas if asluas
    Vehicle* tempUAS = vehicle;

    connect(tempUAS, SIGNAL(MPPTDataChanged(float, float, uint16_t, uint8_t, float, float, uint16_t, uint8_t, float, float, uint16_t, uint8_t)), this, SLOT(updateMPPT(float, float, uint16_t, uint8_t, float, float, uint16_t, uint8_t, float, float, uint16_t, uint8_t)));
    //connect(tempUAS, SIGNAL(MPPTDataChangedHL(uint8_t, uint8_t, uint8_t)), this, SLOT(updateMPPTHL(uint8_t, uint8_t, uint8_t)));
    connect(tempUAS, SIGNAL(BatMonDataChanged(uint8_t, uint16_t, int16_t, uint8_t, float, uint16_t, uint32_t, uint32_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t)), this, SLOT(updateBatMon(uint8_t, uint16_t, int16_t, uint8_t, float, uint16_t, uint32_t, uint32_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t)));
    //connect(tempUAS, SIGNAL(BatMonDataChangedHL(uint8_t, uint8_t, int8_t, uint8_t)), this, SLOT(updateBatMonHL(uint8_t, uint8_t, int8_t, uint8_t)));
    connect(tempUAS, SIGNAL(thrustChanged(Vehicle*, double)), this, SLOT(onThrustChanged(Vehicle*, double)));
    connect(tempUAS, SIGNAL(SensPowerBoardChanged(uint64_t, uint8_t, uint8_t, float, float, float, float, float, float, float, float, float)), this, SLOT(updatePowerBoard(uint64_t, uint8_t, uint8_t, float, float, float, float, float, float, float, float, float)));
}

#define HOSTFETBIT1 "DSG"
#define HOSTFETBIT2 "CHG"
#define HOSTFETBIT3 "PCHG"

QString EnergyBudget::convertHostfet(uint16_t bit)
{
	if (bit & 0x0002) return HOSTFETBIT1;
	if (bit & 0x0004) return HOSTFETBIT2;
	if (bit & 0x0008) return HOSTFETBIT3;
	return "OK";
}

#define BATTERYBit15 "OCA"
#define BATTERYBit14 "TCA"
#define BATTERYBit12 "OTA"
#define BATTERYBit11 "TDA"
#define BATTERYBit9 "RCA"
#define BATTERYBit8 "RTA"
#define BATTERYBit7 "INIT"
#define BATTERYBit6 "DSG"
#define BATTERYBit5 "FC"
#define BATTERYBit4 "FD"

QString EnergyBudget::convertBatteryStatus(uint16_t bit)
{
	if (bit & 0x0010) return BATTERYBit4;
	if (bit & 0x0020) return BATTERYBit5;
	if (bit & 0x0040) return BATTERYBit6;
	if (bit & 0x0080) return BATTERYBit7;
	if (bit & 0x0100) return BATTERYBit8;
	if (bit & 0x0200) return BATTERYBit9;
	if (bit & 0x0800) return BATTERYBit11;
	if (bit & 0x1000) return BATTERYBit12;
	if (bit & 0x4000) return BATTERYBit14;
	if (bit & 0x8000) return BATTERYBit15;
	return "OK";
}

#define MPPTBit0Active "Constant Voltage (CV)"
#define MPPTBit0Inactive "Constant Current (CC)"
#define MPPTBit1 "Overvoltage (OVA)"
#define MPPTBit2 "Overtemperature (OTA)"
#define MPPTBit3 "Overcurrent@L4"
#define MPPTBit4 "Overcurrent@L3"
#define MPPTBit5 "Overcurrent@L2"
#define MPPTBit6 "Overcurrent@L1"

QString EnergyBudget::convertMPPTModus(uint8_t bit)
{
	if (bit & 0x0001) return MPPTBit0Active;
	return MPPTBit0Inactive;
}

QString EnergyBudget::convertMPPTStatus(uint8_t bit)
{
	if (bit & 0x0002) return MPPTBit1;
	if (bit & 0x0004) return MPPTBit2;
	if (bit & 0x0008) return MPPTBit3;
	if (bit & 0x0010) return MPPTBit4;
	if (bit & 0x0020) return MPPTBit5;
	if (bit & 0x0040) return MPPTBit6;
	return "OK";
}

QString EnergyBudget::convertPowerboardStatus(uint8_t statusbit)
{
    if (statusbit & 1<<3) {
        uint8_t FaultyServoChannel= (statusbit & 1<<0)*1 + ((statusbit & 1<<1) > 1)*2 + ((statusbit & 1<<2) > 1)*4 + 1;
        return QString("Servo "+QString::number(FaultyServoChannel)+" faulty");
    } else if(statusbit & 0x0020) {
        return "On backup power";
    } else return "OK";
}

void EnergyBudget::ResetMPPTCmd(void)
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, tr("MPPT reset"), tr("Sending command to reset MPPT. Use this with caution! Are you sure?"), QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::Yes) {
        int MPPTNr = 1; //We only have one MPPT now

        //Send the message via the currently active UAS
        Vehicle* tempUAS = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
        if (tempUAS) {

            mavlink_message_t       msg;
            mavlink_command_long_t  cmd;

            cmd.command = MAV_CMD_RESET_MPPT;
            cmd.confirmation = 0;

            cmd.param1 = (float) MPPTNr;
            cmd.param2 = 0.0f;
            cmd.param3 = 0.0f;
            cmd.param4 = 0.0f;
            cmd.param5 = 0.0f;
            cmd.param6 = 0.0f;
            cmd.param7 = 0.0f;

            cmd.target_system = tempUAS->id();
            cmd.target_component = tempUAS->defaultComponentId();
            mavlink_msg_command_long_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                                 qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                                 tempUAS->priorityLink()->mavlinkChannel(),
                                                 &msg,
                                                 &cmd);

            tempUAS->sendMessageOnLink(tempUAS->priorityLink(), msg);
        }
	}
}

void EnergyBudget::styleChanged(bool darkStyle)
{
	if (darkStyle)
	{
		m_chargePowerText->setDefaultTextColor(QColor(Qt::white));
		m_batUsePowerText->setDefaultTextColor(QColor(Qt::white));
		m_cellPowerText->setDefaultTextColor(QColor(Qt::white));
		m_cellUsePowerText->setDefaultTextColor(QColor(Qt::white));
        m_SystemPowerText->setDefaultTextColor(QColor(Qt::white));
	}
	else
	{
		m_chargePowerText->setDefaultTextColor(QColor(Qt::black));
		m_batUsePowerText->setDefaultTextColor(QColor(Qt::black));
		m_cellPowerText->setDefaultTextColor(QColor(Qt::black));
		m_cellUsePowerText->setDefaultTextColor(QColor(Qt::black));
        m_SystemPowerText->setDefaultTextColor(QColor(Qt::black));
	}
}

void EnergyBudget::MPPTTimerTimeout(void)
{
    m_cellPower = m_SystemPower - m_chargePower - m_batUsePower;
    ui->mppt1VLabel->setText(QString("N/A"));
    ui->mppt1ALabel->setText(QString("N/A"));
}

void EnergyBudget::onThrustChanged(Vehicle* vehicle, double thrust)
{
    Q_UNUSED(vehicle);
	m_thrust = thrust;
}

Hysteresisf::Hysteresisf(float minVal, float maxVal, bool isHighState) :
m_minVal(minVal),
m_maxVal(maxVal),
m_highState(isHighState)
{

}

bool Hysteresisf::check(float const currVal)
{
	if (currVal < m_minVal)
	{
		m_highState = false;
	}
	if (currVal > m_maxVal)
	{
		m_highState = true;
	}
	return m_highState;
}
