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
    //connectButtons();

    //this call functions is only an example to view the color in the groupBox
    changeRedColor(ui->AirSpeedHold_groupBox);
    changeGreenColor(ui->HeightErrorLoPitch_groupBox);
}

SlugsPIDControl::~SlugsPIDControl()
{
    delete ui;
}

/**
 * Set the background color RED of the GroupBox PID based on the send Slugs PID message
 *
 */
void SlugsPIDControl::changeRedColor(QGroupBox *group)
{
    // GroupBox Color
    QColor groupColor = QColor(231,72,28);
    QString colorstyle;
    QString borderColor = "#FA4A4F"; //"#4A4A4F";

    groupColor = groupColor.darker(475);


    colorstyle = colorstyle.sprintf("QGroupBox {background-color: #%02X%02X%02X; border: 5px solid %s; }",
                                    groupColor.red(), groupColor.green(), groupColor.blue(), borderColor.toStdString().c_str());

    group->setStyleSheet(colorstyle);

}

/**
 * Set the background color GREEN of the GroupBox PID based on the send Slugs PID message
 *
 */
void SlugsPIDControl::changeGreenColor(QGroupBox *group)
{
    // GroupBox Color
    QColor groupColor = QColor(30,124,16);
    QString colorstyle;
    QString borderColor = "#24AC23";

    groupColor = groupColor.darker(475);


    colorstyle = colorstyle.sprintf("QGroupBox {background-color: #%02X%02X%02X; border: 5px solid %s; }",
                                    groupColor.red(), groupColor.green(), groupColor.blue(), borderColor.toStdString().c_str());

    group->setStyleSheet(colorstyle);

}

/**
 * Connection Signal and Slot of the set and get buttons on the widget
 *
 */
void SlugsPIDControl::connectButtons()
{
    //ToDo connect buttons set and get. Before create the slots

}


