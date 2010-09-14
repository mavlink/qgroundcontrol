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

    connect(ui->m_orbitalSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setOrbit(double)));
    connect(ui->m_heigthSpinBox, SIGNAL(valueChanged(double)), wp, SLOT(setZ(double)));

    connect(ui->m_orbitCheckBox, SIGNAL(stateChanged(int)), this, SLOT(changeOrbitalState(int)));


    // Read values and set user interface
    updateValues();


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
    ui->m_latitudtextEdit->setText(getLatitudString(wp->getY()));
    ui->m_longitudtextEdit->setText(getLongitudString(wp->getX()));
    ui->idWP_label->setText(QString("%1").arg(wp->getId()));\

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


