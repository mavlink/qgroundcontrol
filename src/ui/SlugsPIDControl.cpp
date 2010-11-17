#include "SlugsPIDControl.h"
#include "ui_SlugsPIDControl.h"


#include <QPalette>
#include<QColor>
#include <QDebug>
#include <UASManager.h>
#include <UAS.h>
#include "LinkManager.h"

SlugsPIDControl::SlugsPIDControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SlugsPIDControl)
{
    ui->setupUi(this);

   connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(activeUasSet(UASInterface*)));

    activeUAS = NULL;


    setRedColorStyle();
    setGreenColorStyle();





//    connect_set_pushButtons();
//    connect_AirSpeed_LineEdit();
//    connect_PitchFollowei_LineEdit();
//    connect_RollControl_LineEdit();
//    connect_HeigthError_LineEdit();
//    connect_YawDamper_LineEdit();
//    connect_Pitch2dT_LineEdit();


}

/**
 * @brief Called when the a new UAS is set to active.
 *
 * Called when the a new UAS is set to active.
 *
 * @param uas The new active UAS
 */
void SlugsPIDControl::activeUasSet(UASInterface* uas)
{
    SlugsMAV* slugsMav = dynamic_cast<SlugsMAV*>(uas);

    if (slugsMav != NULL)
    {
        connect(slugsMav,SIGNAL(slugsActionAck(int,uint8_t,uint8_t)),this,SLOT(recibeMensaje(int,uint8_t,uint8_t)));
    }

    // Set this UAS as active if it is the first one
        if(activeUAS == 0)
        {
            activeUAS = uas;
            systemId = activeUAS->getUASID();
            connect_editLinesPDIvalues();

            //qDebug()<<"------------------->Active UAS ID: "<<uas->getUASID();
        }

}

/**
 * @brief Connect Edition Lines for PID Values
 *
 * @param
 */
void SlugsPIDControl::connect_editLinesPDIvalues()
{
    if(activeUAS)
    {
       connect_set_pushButtons();
       connect_AirSpeed_LineEdit();
       connect_PitchFollowei_LineEdit();
       connect_RollControl_LineEdit();
       connect_HeigthError_LineEdit();
       connect_YawDamper_LineEdit();
       connect_Pitch2dT_LineEdit();
    }
}

SlugsPIDControl::~SlugsPIDControl()
{
    delete ui;
}

/**
 *@brief Set the background color RED style for the GroupBox PID when change lineEdit information
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
 *@brief Set the background color GREEN style for the GroupBox PID when change lineEdit information
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
 *@brief Connection Signal and Slot of the set buttons on the widget
 */
void SlugsPIDControl::connect_set_pushButtons()
{
    //ToDo connect buttons set and get. Before create the slots
   connect(ui->dT_PID_set_pushButton, SIGNAL(clicked()),this,SLOT(changeColor_GREEN_AirSpeed_groupBox()));
   connect(ui->dE_PID_set_pushButton,SIGNAL(clicked()),this,SLOT(changeColor_GREEN_PitchFollowei_groupBox()));
   connect(ui->dA_PID_set_pushButton,SIGNAL(clicked()),this,SLOT(changeColor_GREEN_RollControl_groupBox()));
   connect(ui->HELPComm_PDI_set_pushButton,SIGNAL(clicked()),this,SLOT(changeColor_GREEN_HeigthError_groupBox()));
   connect(ui->dR_PDI_set_pushButton,SIGNAL(clicked()),this,SLOT(changeColor_GREEN_YawDamper_groupBox()));
   connect(ui->Pitch2dT_PDI_set_pushButton,SIGNAL(clicked()),this,SLOT(changeColor_GREEN_Pitch2dT_groupBox()));


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
    //create the packet
    pidMessage.target = activeUAS->getUASID();
    pidMessage.idx = 0;
    pidMessage.pVal = ui->dT_P_set->text().toFloat();
    pidMessage.iVal = ui->dT_I_set->text().toFloat();
    pidMessage.dVal = ui->dT_D_set->text().toFloat();

    UAS *uas = dynamic_cast<UAS*>(UASManager::instance()->getActiveUAS());

    mavlink_message_t msg;

    mavlink_msg_pid_encode(MG::SYSTEM::ID,MG::SYSTEM::COMPID, &msg, &pidMessage);
    uas->sendMessage(msg);



    ui->AirSpeedHold_groupBox->setStyleSheet(GREENcolorStyle);
}


void SlugsPIDControl::connect_PitchFollowei_LineEdit()
{
    connect(ui->dE_P_set, SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_PitchFollowei_groupBox(QString)));
    connect(ui->dE_I_set, SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_PitchFollowei_groupBox(QString)));
    connect(ui->dE_D_set, SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_PitchFollowei_groupBox(QString)));
}

void SlugsPIDControl::changeColor_RED_PitchFollowei_groupBox(QString text)
{
    Q_UNUSED(text);
    ui->PitchFlowei_groupBox->setStyleSheet(REDcolorStyle);

}

void SlugsPIDControl::changeColor_GREEN_PitchFollowei_groupBox()
{
    ui->PitchFlowei_groupBox->setStyleSheet(GREENcolorStyle);
}

// Functions for Roll Control GroupBox
/**
     * @brief Change color style to red when PID values of Roll Control are edited
     *
     *
     * @param
     */
void SlugsPIDControl::changeColor_RED_RollControl_groupBox(QString text)
{
    Q_UNUSED(text);
    ui->RollControl_groupBox->setStyleSheet(REDcolorStyle);
}

/**
         * @brief Change color style to green when PID values of Roll Control are send to UAS
         *
         * @param
         */
void SlugsPIDControl::changeColor_GREEN_RollControl_groupBox()
{
    ui->RollControl_groupBox->setStyleSheet(GREENcolorStyle);
}

/**
         * @brief Connects the SIGNALS from the editline to SLOT RollControl_groupBox
         *
         * @param
         */
void SlugsPIDControl::connect_RollControl_LineEdit()
{
    connect(ui->dA_P_set,SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_RollControl_groupBox(QString)));
    connect(ui->dA_I_set,SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_RollControl_groupBox(QString)));
    connect(ui->dA_D_set,SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_RollControl_groupBox(QString)));
}


// Functions for Heigth Error GroupBox
/**
     * @brief Change color style to red when PID values of Heigth Error are edited
     *
     *
     * @param
     */
void SlugsPIDControl::changeColor_RED_HeigthError_groupBox(QString text)
{
    Q_UNUSED(text);
    ui->HeightErrorLoPitch_groupBox->setStyleSheet(REDcolorStyle);
}

/**
         * @brief Change color style to green when PID values of Heigth Error are send to UAS
         *
         * @param
         */
void SlugsPIDControl::changeColor_GREEN_HeigthError_groupBox()
{
    ui->HeightErrorLoPitch_groupBox->setStyleSheet(GREENcolorStyle);
}

/**
         * @brief Connects the SIGNALS from the editline to SLOT HeigthError_groupBox
         *
         * @param
         */
void SlugsPIDControl::connect_HeigthError_LineEdit()
{
    connect(ui->HELPComm_P_set,SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_HeigthError_groupBox(QString)));
    connect(ui->HELPComm_I_set,SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_HeigthError_groupBox(QString)));
    connect(ui->HELPComm_FF_set,SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_HeigthError_groupBox(QString)));
}


// Functions for Yaw Damper GroupBox
/**
     * @brief Change color style to red when PID values of Yaw Damper are edited
     *
     *
     * @param
     */
void SlugsPIDControl::changeColor_RED_YawDamper_groupBox(QString text)
{
    Q_UNUSED(text);
    ui->YawDamper_groupBox->setStyleSheet(REDcolorStyle);
}

/**
         * @brief Change color style to green when PID values of Yaw Damper are send to UAS
         *
         * @param
         */
void SlugsPIDControl::changeColor_GREEN_YawDamper_groupBox()
{
    ui->YawDamper_groupBox->setStyleSheet(GREENcolorStyle);
}

/**
         * @brief Connects the SIGNALS from the editline to SLOT YawDamper_groupBox
         *
         * @param
         */
void SlugsPIDControl::connect_YawDamper_LineEdit()
{
    connect(ui->dR_P_set,SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_YawDamper_groupBox(QString)));
    connect(ui->dR_I_set,SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_YawDamper_groupBox(QString)));
    connect(ui->dR_D_set,SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_YawDamper_groupBox(QString)));


}

// Functions for Pitch to dT GroupBox
/**
     * @brief Change color style to red when PID values of Pitch to dT are edited
     *
     *
     * @param
     */
void SlugsPIDControl::changeColor_RED_Pitch2dT_groupBox(QString text)
{
    Q_UNUSED(text);
    ui->Pitch2dTFFterm_groupBox->setStyleSheet(REDcolorStyle);
}

/**
         * @brief Change color style to green when PID values of Pitch to dT are send to UAS
         *
         * @param
         */
void SlugsPIDControl::changeColor_GREEN_Pitch2dT_groupBox()
{
    ui->Pitch2dTFFterm_groupBox->setStyleSheet(GREENcolorStyle);
}

/**
         * @brief Connects the SIGNALS from the editline to SLOT Pitch2dT_groupBox
         *
         * @param
         */
void SlugsPIDControl::connect_Pitch2dT_LineEdit()
{
    connect(ui->P2dT_FF_set,SIGNAL(textChanged(QString)),this,SLOT(changeColor_RED_Pitch2dT_groupBox(QString)));
}

void SlugsPIDControl::recibeMensaje(int systemId, uint8_t action, uint8_t result)
{
    ui->recepcion_label->setText(QString::number(action));
}
