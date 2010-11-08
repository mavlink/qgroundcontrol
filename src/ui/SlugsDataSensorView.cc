#include "SlugsDataSensorView.h"
#include "ui_SlugsDataSensorView.h"

#include <UASManager.h>

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

}

SlugsDataSensorView::~SlugsDataSensorView()
{
    delete ui;
}

void SlugsDataSensorView::addUAS(UASInterface* uas)
{
    if (uas != NULL)
    {
        connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(slugLocalPositionChange(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(slugSpeedLocalPositionChanged(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(slugAttitudeChanged(UASInterface*,double,double,double,quint64)));
        connect(uas, SIGNAL(slugsSensorBias(UASInterface*,double,double,double,double,double,double,quint64)), this, SLOT(slugsSensorBiasAcelerometerChanged(UASInterface*,double,double,double,quint64)));

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
            ui->m_SlugsAxBiases_textEdit->setText(QString::number(Axb, 'f', 4));
            ui->m_SlugsAyBiases_textEdit->setText(QString::number(Ayb, 'f', 4));
            ui->m_SlugsAzBiases_textEdit->setText(QString::number(Azb, 'f', 4));
            ui->m_SlugsGxBiases_textEdit->setText(QString::number(Gxb, 'f', 4));
            ui->m_SlugsGyBiases_textEdit->setText(QString::number(Gyb, 'f', 4));
            ui->m_SlugsGzBiases_textEdit->setText(QString::number(Gzb, 'f', 4));

    }

}

void SlugsDataSensorView::slugLocalPositionChanged(UASInterface * uasTemp,
                                                   double x,
                                                   double y,
                                                   double z,
                                                   quint64 time)
{
    Q_UNUSED( uasTemp);

    Xpos = x;
    Ypos = y;
    Zpos = z;
    TimeActualPosition = time;

}

void SlugsDataSensorView::slugSpeedLocalPositionChanged(UASInterface *uasTemp,
                                                        double vx,
                                                        double vy,
                                                        double vz,
                                                        quint64 time)
{
    Q_UNUSED( uasTemp);

    VXpos = vx;
    VYpos = vy;
    VZpos = vz;
    TimeActualSpeed = time;
}

void SlugsDataSensorView::slugAttitudeChanged(UASInterface *uasTemp,
                                              double slugroll,
                                              double slugpitch,
                                              double slugyaw,
                                              quint64 time)
{
    Q_UNUSED( uasTemp);

    roll = slugroll;
    pitch = slugpitch;
    yaw = slugyaw;
    TimeActualAttitude = time;
}

void SlugsDataSensorView::slugsSensorBiasChanged(UASInterface *uasTemp,
                                                 double axb,
                                                 double ayb,
                                                 double azb,
                                                 double gxb,
                                                 double gyb,
                                                 double gzb,
                                                 quint64 time)
{
     Q_UNUSED( uasTemp);

     Axb = axb;
     Ayb = ayb;
     Azb = azb;
     Gxb = gxb;
     Gyb = gyb;
     Gzb = gzb;
     TimeActualBias = time;


}


