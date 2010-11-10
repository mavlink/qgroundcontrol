#include "SlugsDataSensorView.h"
#include "ui_SlugsDataSensorView.h"

#include <UASManager.h>
#include "SlugsMAV.h"

#include <QDebug>

SlugsDataSensorView::SlugsDataSensorView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SlugsDataSensorView)
{
    ui->setupUi(this);


    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    activeUAS = NULL;

    loadParameters();


    this->setVisible(false);

    // timer for refresh UI
    updateTimer = new QTimer(this);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(refresh()));
    updateTimer->start(200);
}

void SlugsDataSensorView::loadParameters()
{

    //Global Position
    Latitude = 0;
    Longitude = 0;
    Height = 0;
    timeGlobalPosition = 0;

    Xpos = 0;
    Ypos = 0;
    Zpos = 0;
    TimeActualPosition = 0;

    VXpos = 0;
    VYpos = 0;
    VZpos = 0;
    TimeActualSpeed =0;

    roll = 0;
    pitch = 0;
    yaw = 0;
    TimeActualAttitude = 0;

    //Sensor Biases
    //Acelerometer
    Axb = 0;
    Ayb = 0;
    Azb = 0;
    //Gyro
    Gxb = 0;
    Gyb = 0;
    Gzb = 0;
    TimeActualBias = 0;

    //Diagnostic Message
    diagFl1 = 0;
    diagFl2 = 0;
    diagFl3 = 0;
    diagSh1 = 0;
    diagSh2 = 0;
    diagSh3 = 0;
    timeDiagnostic = 0;

    //CPU Load
    sensLoad = 0;
    ctrlLoad = 0;
    batVolt = 0;
    timeCpuLoad = 0;

    //navigation
    u_m = 0;
    phi_c = 0;
    theta_c = 0;
    psiDot_c = 0;
    ay_body = 0;
    totalDist = 0;
    dist2Go = 0;
    fromWP = 0;
    toWP = 0;
    timeNavigation = 0;

    // Data Log
    Logfl_1 = 0;
    Logfl_2 = 0;
    Logfl_3 = 0;
    Logfl_4 = 0;
    Logfl_5 = 0;
    Logfl_6 = 0;
    timeDataLog = 0;

    //pwm commands
     dt_c = 0; ///< AutoPilot's throttle command
     dla_c = 0; ///< AutoPilot's left aileron command
     dra_c = 0; ///< AutoPilot's right aileron command
     dr_c  = 0; ///< AutoPilot's rudder command
     dle_c = 0; ///< AutoPilot's left elevator command
     dre_c  = 0; ///< AutoPilot's right elevator command
     dlf_c = 0; ///< AutoPilot's left  flap command
     drf_c = 0; ///< AutoPilot's right flap command
     aux1 = 0; ///< AutoPilot's aux1 command
     aux2 = 0; ///< AutoPilot's aux2 command
     timePWMCommand = 0;


     //filtered data
      aX = 0; ///< Accelerometer X value (m/s^2)
      aY = 0; ///< Accelerometer Y value (m/s^2)
      aZ = 0; ///< Accelerometer Z value (m/s^2)
      gX = 0; ///< Gyro X value (rad/s)
      gY = 0; ///< Gyro Y value (rad/s)
      gZ = 0; ///< Gyro Z value (rad/s)
      mX = 0; ///< Magnetometer X (normalized to 1)
      mY = 0; ///< Magnetometer Y (normalized to 1)
      mZ = 0; ///< Magnetometer Z (normalized to 1)
      timeFiltered = 0;

      //gps date and time
       year = 0  ; ///< Year reported by Gps
       month =0; ///< Month reported by Gps
       day = 0; ///< Day reported by Gps
       hour = 0; ///< Hour reported by Gps
       min = 0; ///< Min reported by Gps
       sec = 0; ///< Sec reported by Gps
       visSat = 0; ///< Visible sattelites reported by Gps
       timeGPSDateTime = 0;


}

SlugsDataSensorView::~SlugsDataSensorView()
{
    delete ui;
}

void SlugsDataSensorView::addUAS(UASInterface* uas)
{
    SlugsMAV* slugsMav = dynamic_cast<SlugsMAV*>(uas);

    if (slugsMav != NULL)
    {
        //connect standar messages
        connect(slugsMav, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(slugLocalPositionChange(UASInterface*,double,double,double,quint64)));
        connect(slugsMav, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(slugSpeedLocalPositionChanged(UASInterface*,double,double,double,quint64)));
        connect(slugsMav, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(slugAttitudeChanged(UASInterface*,double,double,double,quint64)));
        connect(slugsMav, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(slugsGlobalPositionChanged(UASInterface*,double,double,double,quint64)));


        //connect slugs especial messages
        connect(slugsMav, SIGNAL(slugsSensorBias(int,double,double,double,double,double,double,quint64)), this, SLOT(slugsSensorBiasChanged(int,double,double,double,double,double,double,quint64)));
        connect(slugsMav, SIGNAL(slugsDiagnostic(int,double,double,double,int16_t,int16_t,int16_t,quint64)), this, SLOT(slugsDiagnosticMessageChanged(int,double,double,double,int16_t,int16_t,int16_t,quint64)));
        connect(slugsMav, SIGNAL(slugsCPULoad(int,uint8_t,uint8_t,uint8_t,quint64)), this, SLOT(slugsCpuLoadChanged(int,uint8_t,uint8_t,uint8_t,quint64)));
        connect(slugsMav, SIGNAL(slugsNavegation(int,double,double,double,double,double,double,double,uint8_t,uint8_t,quint64)),this,SLOT(slugsNavegationChanged(int,double,double,double,double,double,double,double,uint8_t,uint8_t,quint64)));
        connect(slugsMav, SIGNAL(slugsDataLog(int,double,double,double,double,double,double,quint64)), this, SLOT(slugsDataLogChanged(int,double,double,double,double,double,double,quint64)));
        connect(slugsMav, SIGNAL(slugsPWM(int,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,quint64)),this,SLOT(slugsPWMChanged(int,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,quint64)));
        connect(slugsMav, SIGNAL(slugsFilteredData(int,double,double,double,double,double,double,double,double,double,quint64)),this,SLOT(slugsFilteredDataChanged(int,double,double,double,double,double,double,double,double,double,quint64)));
        connect(slugsMav, SIGNAL(slugsGPSDateTime(int,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,quint64)),this,SLOT(slugsGPSDateTimeChanged(int,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,quint64)));






        // Set this UAS as active if it is the first one
        if(activeUAS == 0)
        {
            activeUAS = uas;
        }
    }
}

void SlugsDataSensorView::setActiveUAS(UASInterface* uas)
{
    activeUAS = uas;


}


void SlugsDataSensorView::refresh()
{
    if(activeUAS)
    {
        //refresh Global Position
        ui->m_GpsLatitude->setText(QString::number(Latitude, 'f', 4));
        ui->m_GpsLongitude->setText(QString::number(Longitude, 'f', 4));
        ui->m_Gpsheigth->setText(QString::number(Height, 'f', 4));

        //refresh GPS Date and Time
        ui->m_GpsDate->setText(QString::number(day)+ "-" + QString::number(month)+ "-" + QString::number(year));
        ui->m_GpsTime->setText(QString::number(hour)+ "-" + QString::number(min)+ "-" + QString::number(sec));
        ui->m_GpsSat->setText(QString::number(visSat));
        ui->m_GpsCog->setText("No Data");
        ui->m_GpsSog->setText("No Data");


            //refresh UI position data
            ui->ed_x->setPlainText(QString::number(Xpos, 'f', 4));
            ui->ed_y->setPlainText(QString::number(Ypos, 'f', 4));
            ui->ed_z->setPlainText(QString::number(Zpos, 'f', 4));

            //refresh UI speed position data
            ui->ed_vx->setPlainText(QString::number(VXpos,'f',4));
            ui->ed_vy->setPlainText(QString::number(VYpos,'f',4));
            ui->ed_vz->setPlainText(QString::number(VZpos,'f',4));

            //refresh UI attitude data
            ui->m_SlugAttitudeRoll_plainTextEdit->setPlainText(QString::number(roll,'f',4));
            ui->m_SlugAttitudePitch_plainTextEdit->setPlainText(QString::number(pitch,'f',4));
            ui->m_SlugAttitudeYaw_plainTextEdit->setPlainText(QString::number(yaw,'f',4));

            //refresh UI sensor bias acelerometer data
            ui->m_AxBiases->setText(QString::number(Axb, 'f', 4));
            ui->m_AyBiases->setText(QString::number(Ayb, 'f', 4));
            ui->m_AzBiases->setText(QString::number(Azb, 'f', 4));
            ui->m_GxBiases->setText(QString::number(Gxb, 'f', 4));
            ui->m_GyBiases->setText(QString::number(Gyb, 'f', 4));
            ui->m_GzBiases->setText(QString::number(Gzb, 'f', 4));

            //refresh UI diagnostic data
            ui->m_SlugsFl1_textEdit->setText(QString::number(diagFl1, 'f', 4));
            ui->m_SlugsFl2_textEdit->setText(QString::number(diagFl2, 'f', 4));
            ui->m_SlugsFl3_textEdit->setText(QString::number(diagFl3, 'f', 4));
            ui->m_SlugsSh1_textEdit->setText(QString::number(diagSh1, 'f', 4));
            ui->m_SlugsSh2_textEdit->setText(QString::number(diagSh2, 'f', 4));
            ui->m_SlugsSh3_textEdit->setText(QString::number(diagSh3, 'f', 4));

            //refresh UI navegation data
            ui->m_SlugsUm_textEdit->setText(QString::number(u_m, 'f', 4));
            ui->m_SlugsPitchC_textEdit->setText(QString::number(pitch, 'f', 4));
            ui->m_SlugsPsidC_textEdit->setText(QString::number(psiDot_c, 'f', 4));
            ui->m_SlugsPhiC_textEdit->setText(QString::number(phi_c, 'f', 4));
            ui->m_SlugsAyBody_textEdit->setText(QString::number(ay_body, 'f', 4));
            ui->m_SlugsFromWP_textEdit->setText(QString::number(fromWP, 'f', 4));
            ui->m_SlugsToWP_textEdit->setText(QString::number(toWP, 'f', 4));
            ui->m_SlugsTotRun_textEdit->setText(QString::number(totalDist, 'f', 4));
            ui->m_SlugsDistToGround_textEdit->setText(QString::number(dist2Go, 'f', 4));

            //refresh UI Data Log
            ui->m_logFl1_textEdit->setText(QString::number(Logfl_1, 'f', 4));
            ui->m_logFl2_textEdit->setText(QString::number(Logfl_2, 'f', 4));
            ui->m_logFl3_textEdit->setText(QString::number(Logfl_3, 'f', 4));
            ui->m_logFl4_textEdit->setText(QString::number(Logfl_4, 'f', 4));
            ui->m_logFl5_textEdit->setText(QString::number(Logfl_5, 'f', 4));
            ui->m_logFl6_textEdit->setText(QString::number(Logfl_6, 'f', 4));

            //refresh UI PWM Commands
            ui->m_pwmThro->setText("No data");
            ui->m_pwmThroTrim->setText("No data");
            ui->m_pwmAile->setText("No data");
            ui->m_pwmAileTrim->setText("No data");
            ui->m_pwmElev->setText("No data");
            ui->m_pwmElevTrim->setText("No data");
            ui->m_pwmRudd->setText("No data");
            ui->m_pwmRuddTrim->setText("No data");
            ui->m_pwmFailSafe->setText("No data");
            ui->m_pwmAvailable->setText("No data");








    }

}

void SlugsDataSensorView::slugsGlobalPositionChanged(UASInterface *uas,
                                                     double lat,
                                                     double lon,
                                                     double alt,
                                                     quint64 time)

{
    Q_UNUSED( uas);
    Latitude = lat;
    Longitude = lon;
    Height = alt;
    timeGlobalPosition = time;



}


void SlugsDataSensorView::slugLocalPositionChanged(UASInterface* uas,
                                                   double x,
                                                   double y,
                                                   double z,
                                                   quint64 time)
{
    Q_UNUSED( uas);

    Xpos = x;
    Ypos = y;
    Zpos = z;
    TimeActualPosition = time;

}

void SlugsDataSensorView::slugSpeedLocalPositionChanged(UASInterface* uas,
                                                        double vx,
                                                        double vy,
                                                        double vz,
                                                        quint64 time)
{
    Q_UNUSED( uas);

    VXpos = vx;
    VYpos = vy;
    VZpos = vz;
    TimeActualSpeed = time;
}

void SlugsDataSensorView::slugAttitudeChanged(UASInterface* uas,
                                              double slugroll,
                                              double slugpitch,
                                              double slugyaw,
                                              quint64 time)
{
    Q_UNUSED( uas);

    roll = slugroll;
    pitch = slugpitch;
    yaw = slugyaw;
    TimeActualAttitude = time;
}

void SlugsDataSensorView::slugsSensorBiasChanged(int systemId,
                                                 double axb,
                                                 double ayb,
                                                 double azb,
                                                 double gxb,
                                                 double gyb,
                                                 double gzb,
                                                 quint64 time)
{

     Q_UNUSED( systemId);

     Axb = axb;
     Ayb = ayb;
     Azb = azb;
     Gxb = gxb;
     Gyb = gyb;
     Gzb = gzb;
     TimeActualBias = time;


}

void SlugsDataSensorView::slugsDiagnosticMessageChanged(int systemId,
                                                        double diagfl1,
                                                        double diagfl2,
                                                        double diagfl3,
                                                        int16_t diagsh1,
                                                        int16_t diagsh2,
                                                        int16_t diagsh3,
                                                        quint64 time)
{
    Q_UNUSED(systemId);

    diagFl1 = diagfl1;
    diagFl2 = diagfl2;
    diagFl3 = diagfl3;
    diagSh1 = diagsh1;
    diagSh2 = diagsh2;
    diagSh3 = diagsh3;
    timeDiagnostic = time;

}


void SlugsDataSensorView::slugsCpuLoadChanged(int systemId,
                                              uint8_t sensload,
                                              uint8_t ctrlload,
                                              uint8_t batvolt,
                                              quint64 time)
{
     Q_UNUSED(systemId);
    sensLoad = sensload;
    ctrlLoad = ctrlload;
    batVolt = batvolt;
    timeCpuLoad = time;
}

void SlugsDataSensorView::slugsNavegationChanged(int systemId,
                                                 double navu_m,
                                                 double navphi_c,
                                                 double navtheta_c,
                                                 double navpsiDot_c,
                                                 double navay_body,
                                                 double navtotalDist,
                                                 double navdist2Go,
                                                 uint8_t navfromWP,
                                                 uint8_t navtoWP,
                                                 quint64 time)
{
     Q_UNUSED(systemId);
     u_m = navu_m;
     phi_c = navphi_c;
     theta_c = navtheta_c;
     psiDot_c = navpsiDot_c;
     ay_body = navay_body;
     totalDist = navtotalDist;
     dist2Go = navdist2Go;
     fromWP = navfromWP;
     toWP = navtoWP;
     timeNavigation = time;
}



void SlugsDataSensorView::slugsDataLogChanged(int systemId,
                                              double logfl_1,
                                              double logfl_2,
                                              double logfl_3,
                                              double logfl_4,
                                              double logfl_5,
                                              double logfl_6,
                                              quint64 time)
{
    qDebug()<<"----------------------------------------------------->>>>>>>>>>>>>>> ACTUALIZANDO LOG DATA";
     Q_UNUSED(systemId);
    Logfl_1 = logfl_1;
    Logfl_2 = logfl_2;
    Logfl_3 = logfl_3;
    Logfl_4 = logfl_4;
    Logfl_5 = logfl_5;
    Logfl_6 = logfl_6;
    timeDataLog = time;
}

void SlugsDataSensorView::slugsPWMChanged(int systemId,
                                          uint16_t vdt_c,
                                          uint16_t vdla_c,
                                          uint16_t vdra_c,
                                          uint16_t vdr_c,
                                          uint16_t vdle_c,
                                          uint16_t vdre_c,
                                          uint16_t vdlf_c,
                                          uint16_t vdrf_c,
                                          uint16_t vda1_c,
                                          uint16_t vda2_c,
                                          quint64 time)
{
       Q_UNUSED(systemId);
       dt_c = vdt_c; ///< AutoPilot's throttle command
       dla_c = vdla_c; ///< AutoPilot's left aileron command
       dra_c = vdra_c; ///< AutoPilot's right aileron command
       dr_c  = vdr_c; ///< AutoPilot's rudder command
       dle_c = vdle_c; ///< AutoPilot's left elevator command
       dre_c  = vdre_c; ///< AutoPilot's right elevator command
       dlf_c = vdlf_c; ///< AutoPilot's left  flap command
       drf_c = vdrf_c; ///< AutoPilot's right flap command
       aux1 = vda1_c; ///< AutoPilot's aux1 command
       aux2 = vda2_c; ///< AutoPilot's aux2 command
       timePWMCommand = time;

}

void SlugsDataSensorView::slugsFilteredDataChanged(int systemId,
                                                   double filaX,
                                                   double filaY,
                                                   double filaZ,
                                                   double filgX,
                                                   double filgY,
                                                   double filgZ,
                                                   double filmX,
                                                   double filmY,
                                                   double filmZ,
                                                   quint64 time)
{
    Q_UNUSED(systemId);

    aX = filaX; ///< Accelerometer X value (m/s^2)
    aY = filaY; ///< Accelerometer Y value (m/s^2)
    aZ = filaZ; ///< Accelerometer Z value (m/s^2)
    gX = filgX; ///< Gyro X value (rad/s)
    gY = filgY; ///< Gyro Y value (rad/s)
    gZ = filgZ; ///< Gyro Z value (rad/s)
    mX = filmX; ///< Magnetometer X (normalized to 1)
    mY = filmY; ///< Magnetometer Y (normalized to 1)
    mZ = filmZ; ///< Magnetometer Z (normalized to 1)
    timeFiltered = time;
}

void SlugsDataSensorView::slugsGPSDateTimeChanged(int systemId,
                                                  uint8_t gpsyear,
                                                  uint8_t gpsmonth,
                                                  uint8_t gpsday,
                                                  uint8_t gpshour,
                                                  uint8_t gpsmin,
                                                  uint8_t gpssec,
                                                  uint8_t gpsvisSat,
                                                  quint64 time)
{
    Q_UNUSED(systemId);

    year = gpsyear  ; ///< Year reported by Gps
    month = gpsmonth; ///< Month reported by Gps
    day = gpsday; ///< Day reported by Gps
    hour = gpshour; ///< Hour reported by Gps
    min = gpsmin; ///< Min reported by Gps
    sec = gpssec; ///< Sec reported by Gps
    visSat = gpsvisSat; ///< Visible sattelites reported by Gps
    timeGPSDateTime = time;

}
