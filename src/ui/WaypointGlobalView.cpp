#include "WaypointGlobalView.h"
#include "ui_WaypointGlobalView.h"

#include <math.h>

WaypointGlobalView::WaypointGlobalView(Waypoint* wp,QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WaypointGlobalView)
{
    ui->setupUi(this);
    this->wp = wp;

    ui->m_orbitalSpinBox->hide();

    // Read values and set user interface
    updateValues();

    connect(ui->m_orbitalSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setOrbit(double)));
    connect(ui->m_heigthSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setZ(double)));

    //for spinBox Latitude
    connect(ui->m_latitudGrados_spinBox, SIGNAL(valueChanged(int)), this, SLOT(updateLatitudeWP(int)));
    connect(ui->m_latitudMinutos_spinBox, SIGNAL(valueChanged(double)), this, SLOT(updateLatitudeMinuteWP(double)));
    connect(ui->m_dirLatitudeN_radioButton, SIGNAL(clicked()), this, SLOT(changeDirectionLatitudeWP()));
    connect(ui->m_dirLatitudeS_radioButton, SIGNAL(clicked()), this, SLOT(changeDirectionLatitudeWP()));

    //for spinBox Longitude
    connect(ui->m_longitudGrados_spinBox, SIGNAL(valueChanged(int)), this, SLOT(updateLongitudeWP(int)));
    connect(ui->m_longitudMinutos_spinBox, SIGNAL(valueChanged(double)), this, SLOT(updateLongitudeMinuteWP(double)));
    connect(ui->m_dirLongitudeW_radioButton, SIGNAL(clicked()), this, SLOT(changeDirectionLongitudeWP()));
    connect(ui->m_dirLongitudeE_radioButton, SIGNAL(clicked()), this, SLOT(changeDirectionLongitudeWP()));



   connect(ui->m_orbitCheckBox, SIGNAL(stateChanged(int)), this, SLOT(changeOrbitalState(int)));





//    connect(m_ui->xSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setX(double)));
//    connect(m_ui->ySpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setY(double)));
//    connect(m_ui->zSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setZ(double)));

//    //hidden degree to radian conversion of the yaw angle
//    connect(m_ui->yawSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setYaw(int)));
//    connect(this, SIGNAL(setYaw(double)), wp, SLOT(setYaw(double)));

//    connect(m_ui->upButton, SIGNAL(clicked()), this, SLOT(moveUp()));
//    connect(m_ui->downButton, SIGNAL(clicked()), this, SLOT(moveDown()));
//    connect(m_ui->removeButton, SIGNAL(clicked()), this, SLOT(remove()));

//    connect(m_ui->autoContinue, SIGNAL(stateChanged(int)), this, SLOT(changedAutoContinue(int)));
//    connect(m_ui->selectedBox, SIGNAL(stateChanged(int)), this, SLOT(changedCurrent(int)));

//
//    connect(m_ui->holdTimeSpinBox, SIGNAL(valueChanged(int)), wp, SLOT(setHoldTime(int)));
}

WaypointGlobalView::~WaypointGlobalView()
{
    delete ui;
}

void WaypointGlobalView::updateValues()
{

            int gradoLat, gradoLon;
            float minLat, minLon;
            QString dirLat, dirLon;

            getLatitudeGradoMin(wp->getY(), &gradoLat, &minLat, &dirLat);
            getLongitudGradoMin(wp->getX(), &gradoLon, &minLon, &dirLon);

            //latitude on spinBox
            ui->m_latitudGrados_spinBox->setValue(gradoLat);
            ui->m_latitudMinutos_spinBox->setValue(minLat);
            if(dirLat == "N")
            {
                ui->m_dirLatitudeN_radioButton->setChecked(true);
                ui->m_dirLatitudeS_radioButton->setChecked(false);
            }
            else
            {
                ui->m_dirLatitudeS_radioButton->setChecked(true);
                ui->m_dirLatitudeN_radioButton->setChecked(false);

            }

             //longitude on spinBox
            ui->m_longitudGrados_spinBox->setValue(gradoLon);
            ui->m_longitudMinutos_spinBox->setValue(minLon);
            if(dirLon == "W")
            {
                ui->m_dirLongitudeW_radioButton->setChecked(true);
                ui->m_dirLongitudeE_radioButton->setChecked(false);
            }
            else
            {
               ui->m_dirLongitudeE_radioButton->setChecked(true);
               ui->m_dirLongitudeW_radioButton->setChecked(false);

            }

            ui->idWP_label->setText(QString("WP-%1").arg(wp->getId()));


}

void WaypointGlobalView::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void WaypointGlobalView::remove()
{
    emit removeWaypoint(wp);
    delete this;
}

QString WaypointGlobalView::getLatitudString(float latitud)
{
   QString tempNS ="";
   QString stringLatitudTemp = "";

   float minutos = 0;
   float grados = 0;
   float entero = 0;
   float dec = 0;

   if (latitud<0){tempNS="S"; latitud = latitud * -1;}
   else {tempNS="N";}

   if(latitud< 90 || latitud > -90)
   {
       dec = latitud - (entero = ::floor(latitud));;
        minutos = dec * 60;
        grados = entero;
        if(grados < 0) grados = grados * (-1);
        if(minutos < 0) minutos = minutos * (-1);

       stringLatitudTemp = QString::number(grados)+ " °  "+ QString::number(minutos)+"'  "+ tempNS;

       return stringLatitudTemp;
   }
   else
   {
       stringLatitudTemp = "erroneous latitude";
       return stringLatitudTemp;
   }

}

QString WaypointGlobalView::getLongitudString(float longitud)
{
    QString tempEW ="";
    QString stringLongitudTemp = "";

    float minutos = 0;
    float grados = 0;
    float entero = 0;
    float dec = 0;

    if (longitud<0){tempEW="W"; longitud = longitud * -1;}
    else {tempEW="E";}

    if(longitud<180 || longitud > -180)
    {
        dec = longitud - (entero = ::floor(longitud));;
         minutos = dec * 60;
         grados = entero;
         if(grados < 0) grados = grados * (-1);
         if(minutos < 0) minutos = minutos * (-1);

        stringLongitudTemp = QString::number(grados)+ " °  "+ QString::number(minutos)+"'  "+ tempEW;

        return stringLongitudTemp;
    }
    else
    {
        stringLongitudTemp = "erroneous longitude";
        return stringLongitudTemp;
    }
}

void WaypointGlobalView::changeOrbitalState(int state)
{
    Q_UNUSED(state);

    if(ui->m_orbitCheckBox->isChecked())
    {
        ui->m_orbitalSpinBox->setEnabled(true);
        ui->m_orbitalSpinBox->show();
    }
    else
    {
        ui->m_orbitalSpinBox->setEnabled(false);
        ui->m_orbitalSpinBox->hide();
    }
}

void WaypointGlobalView::getLatitudeGradoMin(float latitud, int *gradoLat, float *minLat, QString *dirLat)
{

   float minutos = 0;
   float grados = 0;
   float entero = 0;
   float dec = 0;

   if (latitud<0){*dirLat="S"; latitud = latitud * -1;}
   else {*dirLat="N";}

   if(latitud< 90 || latitud > -90)
   {
       dec = latitud - (entero = ::floor(latitud));;
        minutos = dec * 60;
        grados = entero;
        if(grados < 0) grados = grados * (-1);
        if(minutos < 0) minutos = minutos * (-1);

        *gradoLat = grados;
        *minLat = minutos;


   }
   else
   {
       *gradoLat = -1;
       *minLat = -1;
       *dirLat="N/A";

   }

}

void WaypointGlobalView::getLongitudGradoMin(float longitud, int *gradoLon, float *minLon, QString *dirLon)
{

    float minutos = 0;
    float grados = 0;
    float entero = 0;
    float dec = 0;

    if (longitud<0){*dirLon="W"; longitud = longitud * -1;}
    else {*dirLon="E";}

    if(longitud<180 || longitud > -180)
    {
        dec = longitud - (entero = ::floor(longitud));;
         minutos = dec * 60;
         grados = entero;
         if(grados < 0) grados = grados * (-1);
         if(minutos < 0) minutos = minutos * (-1);

         *gradoLon = grados;
         *minLon = minutos;

    }
    else
    {
        *gradoLon = -1;
        *minLon = -1;
        *dirLon="N/A";
    }
}

void WaypointGlobalView::updateCoordValues(float lat, float lon)
{

}

void WaypointGlobalView::updateLatitudeWP(int value)
{
    Q_UNUSED(value);


    int gradoLat;
    float minLat;
    float Latitud;
    QString dirLat;

    gradoLat = ui->m_latitudGrados_spinBox->value();
    minLat = ui->m_latitudMinutos_spinBox->value();
    if(ui->m_dirLatitudeN_radioButton->isChecked())
    {
        dirLat = "N";
    }
    else
    {
        dirLat = "S";
    }
    //dirLat = ui->m_dirLatitud_label->text();

    Latitud = gradoLat + (minLat/60);
    if(dirLat == "S"){Latitud = Latitud * -1;}

    wp->setY(Latitud);

    //emit signal waypoint position was changed

    emit changePositionWP(wp);

}

void WaypointGlobalView::updateLatitudeMinuteWP(double value)
{
    Q_UNUSED(value);

    int gradoLat;
    float minLat;
    float Latitud;
    QString dirLat;

    gradoLat = ui->m_latitudGrados_spinBox->value();
    minLat = ui->m_latitudMinutos_spinBox->value();
    //dirLat = ui->m_dirLatitud_label->text();
    if(ui->m_dirLatitudeN_radioButton->isChecked())
    {
        dirLat = "N";
    }
    else
    {
        dirLat = "S";
    }

    Latitud = gradoLat + (minLat/60);
    if(dirLat == "S"){Latitud = Latitud * -1;}

    wp->setY(Latitud);

    //emit signal waypoint position was changed
    emit changePositionWP(wp);


}

void WaypointGlobalView::updateLongitudeWP(int value)
{
    Q_UNUSED(value);

    int gradoLon;
    float minLon;
    float Longitud;
    QString dirLon;

    gradoLon = ui->m_longitudGrados_spinBox->value();
    minLon = ui->m_longitudMinutos_spinBox->value();
   // dirLon = ui->m_dirLongitud_label->text();
    if(ui->m_dirLongitudeW_radioButton->isChecked())
    {
        dirLon = "W";
    }
    else
    {
        dirLon = "E";
    }

    Longitud = gradoLon + (minLon/60);
    if(dirLon == "W"){Longitud = Longitud * -1;}

    wp->setX(Longitud);

    //emit signal waypoint position was changed
    emit changePositionWP(wp);


}



void WaypointGlobalView::updateLongitudeMinuteWP(double value)
{
    Q_UNUSED(value);

    int gradoLon;
    float minLon;
    float Longitud;
    QString dirLon;

    gradoLon = ui->m_longitudGrados_spinBox->value();
    minLon = ui->m_longitudMinutos_spinBox->value();
   // dirLon = ui->m_dirLongitud_label->text();
    if(ui->m_dirLongitudeW_radioButton->isChecked())
    {
        dirLon = "W";
    }
    else
    {
        dirLon = "E";
    }

    Longitud = gradoLon + (minLon/60);
    if(dirLon == "W"){Longitud = Longitud * -1;}

    wp->setX(Longitud);

    //emit signal waypoint position was changed
    emit changePositionWP(wp);

}

void WaypointGlobalView::changeDirectionLatitudeWP()
{
    if(ui->m_dirLatitudeN_radioButton->isChecked())
    {
        if(wp->getY() < 0)
        {
            wp->setY(wp->getY()* -1);
            //emit signal waypoint position was changed
            emit changePositionWP(wp);
        }

    }
    if(ui->m_dirLatitudeS_radioButton->isChecked())
    {
        if(wp->getY() > 0)
        {
            wp->setY(wp->getY()*-1);
            //emit signal waypoint position was changed
            emit changePositionWP(wp);

        }

    }

}

void WaypointGlobalView::changeDirectionLongitudeWP()
{
    if(ui->m_dirLongitudeW_radioButton->isChecked())
    {
        if(wp->getX() > 0)
        {
           wp->setX(wp->getX()*-1);
           //emit signal waypoint position was changed
           emit changePositionWP(wp);
        }

    }

    if(ui->m_dirLongitudeE_radioButton->isChecked())
    {
        if(wp->getX() < 0)
        {
            wp->setX(wp->getX()*-1);
            //emit signal waypoint position was changed
            emit changePositionWP(wp);
        }

    }



}


