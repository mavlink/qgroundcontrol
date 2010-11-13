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
//    setGreenColorStyle();

    //ORIGINcolorStyle = ui->AirSpeedHold_groupBox->styleSheet();
    //connectButtons();


}

SlugsPIDControl::~SlugsPIDControl()
{
    delete ui;
}

/**
 * Set the background color RED of the GroupBox PID based on the send Slugs PID message
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
 * Set the background color GREEN of the GroupBox PID based on the send Slugs PID message
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
 * Connection Signal and Slot of the set and get buttons on the widget
 *
 */
void SlugsPIDControl::connectButtons()
{
    //ToDo connect buttons set and get. Before create the slots

}

void SlugsPIDControl::connect_AirSpeed_LineEdit()
{
    connect(ui->dT_P_set,SIGNAL(editingFinished()),this, SLOT(changeColor_AirSpeed_groupBox()));
}

void SlugsPIDControl::changeColor_AirSpeed_groupBox()
{
    ui->AirSpeedHold_groupBox->setStyleSheet(REDcolorStyle);

}


