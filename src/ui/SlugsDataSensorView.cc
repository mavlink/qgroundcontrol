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

    activeUAS = NULL;

    this->setVisible(false);




}

SlugsDataSensorView::~SlugsDataSensorView()
{
    delete ui;
}

void SlugsDataSensorView::addUAS(UASInterface* uas)
{
    SlugsMAV* slugsMav = qobject_cast<SlugsMAV*>(uas);

    if (slugsMav != NULL) {

        connect(slugsMav, SIGNAL(slugsRawImu(int, const mavlink_raw_imu_t&)), this, SLOT(slugRawDataChanged(int, const mavlink_raw_imu_t&)));

#ifdef MAVLINK_ENABLED_SLUGS

        //connect standar messages
        connect(slugsMav, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(slugLocalPositionChanged(UASInterface*,double,double,double,quint64)));
        connect(slugsMav, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(slugSpeedLocalPositionChanged(UASInterface*,double,double,double,quint64)));
        connect(slugsMav, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(slugAttitudeChanged(UASInterface*,double,double,double,quint64)));
        connect(slugsMav, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(slugsGlobalPositionChanged(UASInterface*,double,double,double,quint64)));
        connect(slugsMav,SIGNAL(slugsGPSCogSog(int,double,double)),this,SLOT(slugsGPSCogSog(int,double,double)));

        //connect slugs especial messages
        connect(slugsMav, SIGNAL(slugsSensorBias(int,const mavlink_sensor_bias_t&)), this, SLOT(slugsSensorBiasChanged(int,const mavlink_sensor_bias_t&)));
        connect(slugsMav, SIGNAL(slugsDiagnostic(int,const mavlink_diagnostic_t&)), this, SLOT(slugsDiagnosticMessageChanged(int,const mavlink_diagnostic_t&)));
        connect(slugsMav, SIGNAL(slugsCPULoad(int,const mavlink_cpu_load_t&)), this, SLOT(slugsCpuLoadChanged(int,const mavlink_cpu_load_t&)));

        connect(slugsMav, SIGNAL(slugsNavegation(int,const mavlink_slugs_navigation_t&)),this,SLOT(slugsNavegationChanged(int,const mavlink_slugs_navigation_t&)));
        connect(slugsMav, SIGNAL(slugsDataLog(int,const mavlink_data_log_t&)), this, SLOT(slugsDataLogChanged(int,const mavlink_data_log_t&)));
//    connect(slugsMav, SIGNAL(slugsPWM(int,const mavlink_pwm_commands_t&)),this,SLOT(slugsPWMChanged(int,const mavlink_pwm_commands_t&)));
//    connect(slugsMav, SIGNAL(slugsFilteredData(int,const mavlink_filtered_data_t&)),this,SLOT(slugsFilteredDataChanged(int,const mavlink_filtered_data_t&)));
        connect(slugsMav, SIGNAL(slugsGPSDateTime(int,const mavlink_gps_date_time_t&)),this,SLOT(slugsGPSDateTimeChanged(int,const mavlink_gps_date_time_t&)));
        connect(slugsMav,SIGNAL(slugsAirData(int, const mavlink_air_data_t&)),this,SLOT(slugsAirDataChanged(int, const mavlink_air_data_t&)));


        connect(slugsMav, SIGNAL(slugsChannels(int, const mavlink_rc_channels_raw_t&)), this, SLOT(slugsRCRawChannels(int, const mavlink_rc_channels_raw_t&)));
        connect(slugsMav, SIGNAL(slugsServo(int, const mavlink_servo_output_raw_t&)), this, SLOT(slugsRCServo(int,const mavlink_servo_output_raw_t&)));
        connect(slugsMav, SIGNAL(slugsScaled(int, const mavlink_scaled_imu_t&)), this, SLOT(slugsFilteredDataChanged(int, const mavlink_scaled_imu_t&)));

#endif // MAVLINK_ENABLED_SLUGS
        // Set this UAS as active if it is the first one
        if(activeUAS == 0) {
            activeUAS = uas;
        }

    }
}

void SlugsDataSensorView::slugRawDataChanged(int uasId, const mavlink_raw_imu_t &rawData)
{
    Q_UNUSED(uasId);

    ui->m_Axr->setText(QString::number(rawData.xacc));
    ui->m_Ayr->setText(QString::number(rawData.yacc));
    ui->m_Azr->setText(QString::number(rawData.zacc));

    ui->m_Mxr->setText(QString::number(rawData.xmag));
    ui->m_Myr->setText(QString::number(rawData.ymag));
    ui->m_Mzr->setText(QString::number(rawData.zmag));

    ui->m_Gxr->setText(QString::number(rawData.xgyro));
    ui->m_Gyr->setText(QString::number(rawData.ygyro));
    ui->m_Gzr->setText(QString::number(rawData.zgyro));

}

#ifdef MAVLINK_ENABLED_SLUGS
void SlugsDataSensorView::slugsRCRawChannels(int systemId, const mavlink_rc_channels_raw_t &gpsDateTime)
{
    Q_UNUSED(systemId);

    ui->tbRCThrottle->setText(QString::number(gpsDateTime.chan1_raw));
    ui->tbRCAileron->setText(QString::number(gpsDateTime.chan2_raw));
    ui->tbRCRudder->setText(QString::number(gpsDateTime.chan3_raw));
    ui->tbRCElevator->setText(QString::number(gpsDateTime.chan4_raw));
}

void SlugsDataSensorView::slugsRCServo(int systemId, const mavlink_servo_output_raw_t &gpsDateTime)
{
    Q_UNUSED(systemId);

    ui->m_pwmThro->setText(QString::number(gpsDateTime.servo1_raw));
    ui->m_pwmAile->setText(QString::number(gpsDateTime.servo2_raw));
    ui->m_pwmRudd->setText(QString::number(gpsDateTime.servo3_raw));
    ui->m_pwmElev->setText(QString::number(gpsDateTime.servo4_raw));

    ui->m_pwmThroTrim->setText(QString::number(gpsDateTime.servo5_raw));
    ui->m_pwmAileTrim->setText(QString::number(gpsDateTime.servo6_raw));
    ui->m_pwmRuddTrim->setText(QString::number(gpsDateTime.servo7_raw));
    ui->m_pwmElevTrim->setText(QString::number(gpsDateTime.servo8_raw));
}
#endif

void SlugsDataSensorView::setActiveUAS(UASInterface* uas)
{
    activeUAS = uas;
    addUAS(activeUAS);
}

#ifdef MAVLINK_ENABLED_SLUGS

void SlugsDataSensorView::slugsGlobalPositionChanged(UASInterface *uas,
        double lat,
        double lon,
        double alt,
        quint64 time)
{
    Q_UNUSED(uas);
    Q_UNUSED(time);

    ui->m_GpsLatitude->setText(QString::number(lat));
    ui->m_GpsLongitude->setText(QString::number(lon));
    ui->m_GpsHeight->setText(QString::number(alt));

//qDebug()<<"GPS Position = "<<lat<<" - "<<lon<<" - "<<alt;
}


void SlugsDataSensorView::slugLocalPositionChanged(UASInterface* uas,
        double x,
        double y,
        double z,
        quint64 time)
{
    Q_UNUSED(uas);
    Q_UNUSED(time);

    ui->ed_x->setText(QString::number(x));
    ui->ed_y->setText(QString::number(y));
    ui->ed_z->setText(QString::number(z));

}

void SlugsDataSensorView::slugSpeedLocalPositionChanged(UASInterface* uas,
        double vx,
        double vy,
        double vz,
        quint64 time)
{
    Q_UNUSED( uas);
    Q_UNUSED(time);

    ui->ed_vx->setText(QString::number(vx));
    ui->ed_vy->setText(QString::number(vy));
    ui->ed_vz->setText(QString::number(vz));

    //qDebug()<<"Speed Local Position = "<<vx<<" - "<<vy<<" - "<<vz;


}

void SlugsDataSensorView::slugAttitudeChanged(UASInterface* uas,
        double slugroll,
        double slugpitch,
        double slugyaw,
        quint64 time)
{
    Q_UNUSED( uas);
    Q_UNUSED(time);

    ui->m_Roll->setText(QString::number(slugroll));
    ui->m_Pitch->setText(QString::number(slugpitch));
    ui->m_Yaw->setText(QString::number(slugyaw));

    // qDebug()<<"Attitude change = "<<slugroll<<" - "<<slugpitch<<" - "<<slugyaw;

}


void SlugsDataSensorView::slugsSensorBiasChanged(int systemId,
        const mavlink_sensor_bias_t& sensorBias)
{
    Q_UNUSED( systemId);

    ui->m_AxBiases->setText(QString::number(sensorBias.axBias));
    ui->m_AyBiases->setText(QString::number(sensorBias.ayBias));
    ui->m_AzBiases->setText(QString::number(sensorBias.azBias));
    ui->m_GxBiases->setText(QString::number(sensorBias.gxBias));
    ui->m_GyBiases->setText(QString::number(sensorBias.gyBias));
    ui->m_GzBiases->setText(QString::number(sensorBias.gzBias));

}


void SlugsDataSensorView::slugsDiagnosticMessageChanged(int systemId,
        const mavlink_diagnostic_t& diagnostic)
{
    Q_UNUSED(systemId);

    ui->m_Fl1->setText(QString::number(diagnostic.diagFl1));
    ui->m_Fl2->setText(QString::number(diagnostic.diagFl2));
    ui->m_Fl3->setText(QString::number(diagnostic.diagFl2));

    ui->m_Sh1->setText(QString::number(diagnostic.diagSh1));
    ui->m_Sh2->setText(QString::number(diagnostic.diagSh2));
    ui->m_Sh3->setText(QString::number(diagnostic.diagSh3));
}


void SlugsDataSensorView::slugsCpuLoadChanged(int systemId,
        const mavlink_cpu_load_t& cpuLoad)
{
    Q_UNUSED(systemId);
    ui->ed_sens->setText(QString::number(cpuLoad.sensLoad));
    ui->ed_control->setText(QString::number(cpuLoad.ctrlLoad));
    ui->ed_batvolt->setText(QString::number(cpuLoad.batVolt));
}

void SlugsDataSensorView::slugsNavegationChanged(int systemId,
        const mavlink_slugs_navigation_t& slugsNavigation)
{
    Q_UNUSED(systemId);
    ui->m_Um->setText(QString::number(slugsNavigation.u_m));
    ui->m_PhiC->setText(QString::number(slugsNavigation.phi_c));
    ui->m_PitchC->setText(QString::number(slugsNavigation.theta_c));
    ui->m_PsidC->setText(QString::number(slugsNavigation.psiDot_c));
    ui->m_AyBody->setText(QString::number(slugsNavigation.ay_body));
    ui->m_TotRun->setText(QString::number(slugsNavigation.totalDist));
    ui->m_DistToGo->setText(QString::number(slugsNavigation.dist2Go));
    ui->m_FromWP->setText(QString::number(slugsNavigation.fromWP));
    ui->m_ToWP->setText(QString::number(slugsNavigation.toWP));
}



void SlugsDataSensorView::slugsDataLogChanged(int systemId,
        const mavlink_data_log_t& dataLog)
{
    Q_UNUSED(systemId);
    ui->m_logFl1->setText(QString::number(dataLog.fl_1));
    ui->m_logFl2->setText(QString::number(dataLog.fl_2));
    ui->m_logFl3->setText(QString::number(dataLog.fl_3));
    ui->m_logFl4->setText(QString::number(dataLog.fl_4));
    ui->m_logFl5->setText(QString::number(dataLog.fl_5));
    ui->m_logFl6->setText(QString::number(dataLog.fl_6));
}

//void SlugsDataSensorView::slugsPWMChanged(int systemId,
//                                          const mavlink_servo_output_raw_t& pwmCommands){
//       Q_UNUSED(systemId);
//  ui->m_pwmThro->setText(QString::number(pwmCommands.servo1_raw));//.dt_c));
//  ui->m_pwmAile->setText(QString::number(pwmCommands.servo2_raw));//dla_c));
//  ui->m_pwmElev->setText(QString::number(pwmCommands.servo4_raw));//dle_c));
//  ui->m_pwmRudd->setText(QString::number(pwmCommands.servo3_raw));//dr_c));

//  ui->m_pwmThroTrim->setText(QString::number(pwmCommands.servo5_raw));//dre_c));
//  ui->m_pwmAileTrim->setText(QString::number(pwmCommands.servo6_raw));//dlf_c));
//  ui->m_pwmElevTrim->setText(QString::number(pwmCommands.servo8_raw));//drf_c));
//  ui->m_pwmRuddTrim->setText(QString::number(pwmCommands.servo7_raw));//aux1));

//}

void SlugsDataSensorView::slugsFilteredDataChanged(int systemId,
        const mavlink_scaled_imu_t& filteredData)
{
    Q_UNUSED(systemId);
    ui->m_Axf->setText(QString::number(filteredData.xacc/1000.0f));
    ui->m_Ayf->setText(QString::number(filteredData.yacc/1000.0f));
    ui->m_Azf->setText(QString::number(filteredData.zacc/1000.0f));
    ui->m_Gxf->setText(QString::number(filteredData.xgyro/1000.0f));
    ui->m_Gyf->setText(QString::number(filteredData.ygyro/1000.0f));
    ui->m_Gzf->setText(QString::number(filteredData.zgyro/1000.0f));
    ui->m_Mxf->setText(QString::number(filteredData.xmag/1000.0f));
    ui->m_Myf->setText(QString::number(filteredData.ymag/1000.0f));
    ui->m_Mzf->setText(QString::number(filteredData.zmag/1000.0f));
}

void SlugsDataSensorView::slugsGPSDateTimeChanged(int systemId,
        const mavlink_gps_date_time_t& gpsDateTime)
{
    Q_UNUSED(systemId);

    QString month, day;

    month = QString::number(gpsDateTime.month);
    day = QString::number(gpsDateTime.day);

    if(gpsDateTime.month < 10) month = "0" + QString::number(gpsDateTime.month);
    if(gpsDateTime.day < 10) day = "0" + QString::number(gpsDateTime.day);


    ui->m_GpsDate->setText(day + "/" +
                           month + "/" +
                           QString::number(gpsDateTime.year));

    QString hour, min, sec;

    hour = QString::number(gpsDateTime.hour);
    min =  QString::number(gpsDateTime.min);
    sec =  QString::number(gpsDateTime.sec);

    if(gpsDateTime.hour < 10) hour = "0" + QString::number(gpsDateTime.hour);
    if(gpsDateTime.min < 10) min = "0" + QString::number(gpsDateTime.min);
    if(gpsDateTime.sec < 10) sec = "0" + QString::number(gpsDateTime.sec);

    ui->m_GpsTime->setText(hour + ":" +
                           min + ":" +
                           sec);

    ui->m_GpsSat->setText(QString::number(gpsDateTime.visSat));


}

/**
     * @brief Updates the air data widget - 171
*/
void SlugsDataSensorView::slugsAirDataChanged(int systemId, const mavlink_air_data_t &airData)
{
    Q_UNUSED(systemId);
    ui->ed_dynamic->setText(QString::number(airData.dynamicPressure));
    ui->ed_static->setText(QString::number(airData.staticPressure));
    ui->ed_temp->setText(QString::number(airData.temperature));
}

/**
     * @brief set COG and SOG values
     *
     * COG and SOG GPS display on the Widgets
*/
void SlugsDataSensorView::slugsGPSCogSog(int systemId, double cog, double sog)
{
    Q_UNUSED(systemId);

    ui->m_GpsCog->setText(QString::number(cog));
    ui->m_GpsSog->setText(QString::number(sog));
}

#endif // MAVLINK_ENABLED_SLUGS
