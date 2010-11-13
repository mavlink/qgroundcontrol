#include "SlugsPIDControl.h"
#include "ui_SlugsPIDControl.h"


#include <QPalette>
#include<QColor>
#include <QDebug>

SlugsPIDControl::SlugsPIDControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SlugsPIDControl)
{
    ui->setupUi(this);
    setRedColorStyle();
    setGreenColorStyle();


    connect_set_pushButtons();
    connect_AirSpeed_LineEdit();


}

SlugsPIDControl::~SlugsPIDControl()
{
    delete ui;
}

/**
 * Set the background color RED style for the GroupBox PID when change lineEdit information
 *
 */
void SlugsPIDControl::setRedColorStyle()
{
    // GroupBox Color
    QColor groupColor = QColor(231,72,28);

    QString borderColor = "#FA4A4F";

    groupColor = groupColor.darker(475);


    REDcolorStyle = REDcolorStyle.sprintf("QGroupBox {background-color: #%02X%02X%02X; border: 5px solid %s; }",
                                    groupColor.red(), groupColor.green(), groupColor.blue(), borderColor.toStdString().c_str());

}

/**
 * Set the background color GREEN style for the GroupBox PID when change lineEdit information
 *
 */
void SlugsPIDControl::setGreenColorStyle()
{
    // create Green color style
    QColor groupColor = QColor(30,124,16);
    QString borderColor = "#24AC23";

    groupColor = groupColor.darker(475);


    GREENcolorStyle = GREENcolorStyle.sprintf("QGroupBox {background-color: #%02X%02X%02X; border: 5px solid %s; }",
                                    groupColor.red(), groupColor.green(), groupColor.blue(), borderColor.toStdString().c_str());

}

/**
 * Connection Signal and Slot of the set buttons on the widget
 */
void SlugsPIDControl::connect_set_pushButtons()
{
    //ToDo connect buttons set and get. Before create the slots
    connect(ui->dT_PID_set_pushButton, SIGNAL(clicked(bool)),this,SLOT(changeColor_GREEN_AirSpeed_groupBox()));


}

void SlugsPIDControl::connect_AirSpeed_LineEdit()
{
    connect(ui->dT_P_set,SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_AirSpeed_groupBox(QString)));
    connect(ui->dT_I_set,SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_AirSpeed_groupBox(QString)));
    connect(ui->dT_D_set,SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_AirSpeed_groupBox(QString)));
}

void SlugsPIDControl::changeColor_RED_AirSpeed_groupBox(QString text)
{
    Q_UNUSED(text);
    ui->AirSpeedHold_groupBox->setStyleSheet(REDcolorStyle);

}

void SlugsPIDControl::changeColor_GREEN_AirSpeed_groupBox()
{
    ui->AirSpeedHold_groupBox->setStyleSheet(GREENcolorStyle);
}


