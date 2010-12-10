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

    refreshTimerGet = new QTimer(this);
    refreshTimerGet->setInterval(100); // 20 Hz
    connect(refreshTimerGet, SIGNAL(timeout()), this, SLOT(slugsGetGeneral()));


    refreshTimerSet = new QTimer(this);
    refreshTimerSet->setInterval(100); // 20 Hz
    connect(refreshTimerSet, SIGNAL(timeout()), this, SLOT(slugsSetGeneral()));


    counterRefreshGet = 1;
    counterRefreshSet = 1;

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
    #ifdef MAVLINK_ENABLED_SLUGS
    SlugsMAV* slugsMav = dynamic_cast<SlugsMAV*>(uas);

    if (slugsMav != NULL)
    {
        connect(slugsMav,SIGNAL(slugsActionAck(int,const mavlink_action_ack_t&)),this,SLOT(recibeMensaje(int,mavlink_action_ack_t)));
        connect(slugsMav,SIGNAL(slugsPidValues(int,mavlink_pid_t)),this, SLOT(receivePidValues(int,mavlink_pid_t)) );

        connect(ui->setGeneral_pushButton,SIGNAL(clicked()),this,SLOT(slugsTimerStartSet()));
        connect(ui->getGeneral_pushButton,SIGNAL(clicked()),this,SLOT(slugsTimerStartGet()));
    }

#endif // MAVLINK_ENABLED_SLUG
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
       connect_get_pushButtons();
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

/**
 *@brief Connection Signal and Slot of the set buttons on the widget
 */
void SlugsPIDControl::connect_get_pushButtons()
{
   connect(ui->dT_PID_get_pushButton, SIGNAL(clicked()),this,SLOT(get_AirSpeed_PID()));
   connect(ui->dE_PID_get_pushButton,SIGNAL(clicked()),this,SLOT(get_PitchFollowei_PID()));
   connect(ui->dR_PDI_get_pushButton,SIGNAL(clicked()),this,SLOT(get_YawDamper_PID()));
   connect(ui->dA_PID_get_pushButton,SIGNAL(clicked()),this,SLOT(get_RollControl_PID()));
   connect(ui->Pitch2dT_PDI_get_pushButton,SIGNAL(clicked()),this,SLOT(get_Pitch2dT_PID()));
   connect(ui->HELPComm_PDI_get_pushButton,SIGNAL(clicked()),this,SLOT(get_HeigthError_PID()));

}

// Functions for Air Speed GroupBox
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

  SlugsMAV* slugsMav = dynamic_cast<SlugsMAV*>(activeUAS);

  if (slugsMav != NULL)
  {

    //create the packet
#ifdef MAVLINK_ENABLED_SLUGS
    pidMessage.target = activeUAS->getUASID();
    pidMessage.idx = 0;
    pidMessage.pVal = ui->dT_P_set->text().toFloat();
    pidMessage.iVal = ui->dT_I_set->text().toFloat();
    pidMessage.dVal = ui->dT_D_set->text().toFloat();

    mavlink_message_t msg;

    mavlink_msg_pid_encode(MG::SYSTEM::ID,MG::SYSTEM::COMPID, &msg, &pidMessage);
    slugsMav->sendMessage(msg);
#endif

    ui->AirSpeedHold_groupBox->setStyleSheet(GREENcolorStyle);
  }
}

void SlugsPIDControl::get_AirSpeed_PID()
{
     qDebug() << "\nSend Message = Air Speed ";
   sendMessagePIDStatus(0);

}



// Functions for PitchFollowei GroupBox
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
    SlugsMAV* slugsMav = dynamic_cast<SlugsMAV*>(activeUAS);

    if (slugsMav != NULL)
    {
        #ifdef MAVLINK_ENABLED_SLUGS
      //create the packet
      pidMessage.target = activeUAS->getUASID();
      pidMessage.idx = 2;
      pidMessage.pVal = ui->dE_P_set->text().toFloat();
      pidMessage.iVal = ui->dE_I_set->text().toFloat();
      pidMessage.dVal = ui->dE_D_set->text().toFloat();

      mavlink_message_t msg;

      mavlink_msg_pid_encode(MG::SYSTEM::ID,MG::SYSTEM::COMPID, &msg, &pidMessage);
      slugsMav->sendMessage(msg);
#endif

      ui->PitchFlowei_groupBox->setStyleSheet(GREENcolorStyle);
  }
}

void SlugsPIDControl::get_PitchFollowei_PID()
{
    qDebug() << "\nSend Message = Pitch Followei ";
  sendMessagePIDStatus(2);

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
    SlugsMAV* slugsMav = dynamic_cast<SlugsMAV*>(activeUAS);

    if (slugsMav != NULL)
    {
        #ifdef MAVLINK_ENABLED_SLUGS
          //create the packet
          pidMessage.target = activeUAS->getUASID();
          pidMessage.idx = 4;
          pidMessage.pVal = ui->dA_P_set->text().toFloat();
          pidMessage.iVal = ui->dA_I_set->text().toFloat();
          pidMessage.dVal = ui->dA_D_set->text().toFloat();

          mavlink_message_t msg;

          mavlink_msg_pid_encode(MG::SYSTEM::ID,MG::SYSTEM::COMPID, &msg, &pidMessage);
          slugsMav->sendMessage(msg);
#endif

        ui->RollControl_groupBox->setStyleSheet(GREENcolorStyle);
    }
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

void SlugsPIDControl::get_RollControl_PID()
{
    qDebug() << "\nSend Message = Roll Control ";
  sendMessagePIDStatus(4);
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
    SlugsMAV* slugsMav = dynamic_cast<SlugsMAV*>(activeUAS);

    if (slugsMav != NULL)
    {
        #ifdef MAVLINK_ENABLED_SLUGS
          //create the packet
          pidMessage.target = activeUAS->getUASID();
          pidMessage.idx = 1;
          pidMessage.pVal = ui->HELPComm_P_set->text().toFloat();
          pidMessage.iVal = ui->HELPComm_I_set->text().toFloat();
          pidMessage.dVal = ui->HELPComm_FF_set->text().toFloat();

          mavlink_message_t msg;

          mavlink_msg_pid_encode(MG::SYSTEM::ID,MG::SYSTEM::COMPID, &msg, &pidMessage);
          slugsMav->sendMessage(msg);
#endif

        ui->HeightErrorLoPitch_groupBox->setStyleSheet(GREENcolorStyle);
    }
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

void SlugsPIDControl::get_HeigthError_PID()
{
    qDebug() << "\nSend Message = Heigth Error ";
  sendMessagePIDStatus(1);
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
    SlugsMAV* slugsMav = dynamic_cast<SlugsMAV*>(activeUAS);

    if (slugsMav != NULL)
    {
        #ifdef MAVLINK_ENABLED_SLUGS
          //create the packet
          pidMessage.target = activeUAS->getUASID();
          pidMessage.idx = 3;
          pidMessage.pVal = ui->dR_P_set->text().toFloat();
          pidMessage.iVal = ui->dR_I_set->text().toFloat();
          pidMessage.dVal = ui->dR_D_set->text().toFloat();

          mavlink_message_t msg;

          mavlink_msg_pid_encode(MG::SYSTEM::ID,MG::SYSTEM::COMPID, &msg, &pidMessage);
          slugsMav->sendMessage(msg);
#endif

        ui->YawDamper_groupBox->setStyleSheet(GREENcolorStyle);
    }
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

void SlugsPIDControl::get_YawDamper_PID()
{
    qDebug() << "\nSend Message = Yaw Damper ";
  sendMessagePIDStatus(3);
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
    SlugsMAV* slugsMav = dynamic_cast<SlugsMAV*>(activeUAS);

    if (slugsMav != NULL)
    {
#ifdef MAVLINK_ENABLED_SLUGS
          //create the packet
          pidMessage.target = activeUAS->getUASID();
          pidMessage.idx = 8;
          pidMessage.pVal = ui->P2dT_FF_set->text().toFloat();
          pidMessage.iVal = 0;//ui->dR_I_set->text().toFloat();
          pidMessage.dVal = 0;//ui->dR_D_set->text().toFloat();

          mavlink_message_t msg;

          mavlink_msg_pid_encode(MG::SYSTEM::ID,MG::SYSTEM::COMPID, &msg, &pidMessage);
          slugsMav->sendMessage(msg);
#endif
        ui->Pitch2dTFFterm_groupBox->setStyleSheet(GREENcolorStyle);
    }
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

void SlugsPIDControl::get_Pitch2dT_PID()
{
    qDebug() << "\nSend Message = Pitch to dT ";
  sendMessagePIDStatus(8);
}

#ifdef MAVLINK_ENABLED_SLUGS

void SlugsPIDControl::recibeMensaje(int systemId, const mavlink_action_ack_t& action)
{
    ui->recepcion_label->setText(QString::number(action.action) + ":" + QString::number(action.result));
}

void SlugsPIDControl::receivePidValues(int systemId, const mavlink_pid_t &pidValues)
{
    Q_UNUSED(systemId);

     qDebug() << "\nACTUALIZANDO GUI = " << pidValues.idx;
    switch(pidValues.idx)
    {
    case 0:
        ui->dT_P_get->setText(QString::number(pidValues.pVal));
        ui->dT_I_get->setText(QString::number(pidValues.iVal));
        ui->dT_D_get->setText(QString::number(pidValues.dVal));
        break;
    case 1:
        ui->HELPComm_P_get->setText(QString::number(pidValues.pVal));
        ui->HELPComm_I_get->setText(QString::number(pidValues.iVal));
        ui->HELPComm_FF_get->setText(QString::number(pidValues.dVal));
        break;
    case 2:
        ui->dE_P_get->setText(QString::number(pidValues.pVal));
        ui->dE_I_get->setText(QString::number(pidValues.iVal));
        ui->dE_D_get->setText(QString::number(pidValues.dVal));
        break;
    case 3:
        ui->dR_P_get->setText(QString::number(pidValues.pVal));
        ui->dR_I_get->setText(QString::number(pidValues.iVal));
        ui->dR_D_get->setText(QString::number(pidValues.dVal));
        break;
    case 4:
        ui->dA_P_get->setText(QString::number(pidValues.pVal));
        ui->dA_I_get->setText(QString::number(pidValues.iVal));
        ui->dA_D_get->setText(QString::number(pidValues.dVal));
        break;
    case 8:
        ui->P2dT_FF_get->setText(QString::number(pidValues.pVal));

        break;

    default:
        qDebug() << "\nSLUGS RECEIVED AND SHOW PID type ID = " << pidValues.idx;
          break;

    }
}
#endif // MAVLINK_ENABLED_SLUG


void SlugsPIDControl::sendMessagePIDStatus(int PIDtype)
{
#ifdef MAVLINK_ENABLED_SLUGS
    //ToDo remplace actionId values


    SlugsMAV* slugsMav = dynamic_cast<SlugsMAV*>(activeUAS);

    if (slugsMav != NULL)
    {
        mavlink_message_t msg;
        qDebug() << "\n Send Message SLUGS PID with loop index  = " << PIDtype;

        switch(PIDtype)
        {
            case 0: //Air Speed PID values Request
                actionSlugs.target = activeUAS->getUASID();
                actionSlugs.actionId = 9;
                actionSlugs.actionVal = 0;



                mavlink_msg_slugs_action_encode(MG::SYSTEM::ID,MG::SYSTEM::COMPID, &msg, &actionSlugs);
                slugsMav->sendMessage(msg);
                break;

            case 1: //Heigth Error lo Pitch Comm PID values request
                actionSlugs.target = activeUAS->getUASID();
                actionSlugs.actionId = 9;
                actionSlugs.actionVal = 1;


                mavlink_msg_slugs_action_encode(MG::SYSTEM::ID,MG::SYSTEM::COMPID, &msg, &actionSlugs);
                slugsMav->sendMessage(msg);
                break;

            case 2://Pitch Followei PID values Request
                actionSlugs.target = activeUAS->getUASID();
                actionSlugs.actionId = 9;
                actionSlugs.actionVal = 2;


                mavlink_msg_slugs_action_encode(MG::SYSTEM::ID,MG::SYSTEM::COMPID, &msg, &actionSlugs);
                slugsMav->sendMessage(msg);
                break;

            case 3:// Yaw Damper PID values  request
                actionSlugs.target = activeUAS->getUASID();
                actionSlugs.actionId = 9;
                actionSlugs.actionVal = 3;


                mavlink_msg_slugs_action_encode(MG::SYSTEM::ID,MG::SYSTEM::COMPID, &msg, &actionSlugs);
                slugsMav->sendMessage(msg);
                break;

            case 4: // Roll Control PID values request
                actionSlugs.target = activeUAS->getUASID();
                actionSlugs.actionId = 9;
                actionSlugs.actionVal = 4;


                mavlink_msg_slugs_action_encode(MG::SYSTEM::ID,MG::SYSTEM::COMPID, &msg, &actionSlugs);
                slugsMav->sendMessage(msg);
                break;

            case 8: // Pitch to dT FF term
                actionSlugs.target = activeUAS->getUASID();
                actionSlugs.actionId = 9;
                actionSlugs.actionVal = 8;


                mavlink_msg_slugs_action_encode(MG::SYSTEM::ID,MG::SYSTEM::COMPID, &msg, &actionSlugs);
                slugsMav->sendMessage(msg);
                break;


            default:
                qDebug() << "\nSLUGS RECEIVED PID type ID = " << PIDtype;
                  break;


        }
    }
    #endif // MAVLINK_ENABLED_SLUG
}

void SlugsPIDControl::slugsGetGeneral()
{
    valuesMutex.lock();
    switch(counterRefreshGet)
    {
    case 1:
       ui->dT_PID_get_pushButton->click();
        break;
    case 2:
        ui->HELPComm_PDI_get_pushButton->click();
        break;
    case 3:
        ui->dE_PID_get_pushButton->click();
        break;
    case 4:
        ui->dR_PDI_get_pushButton->click();
        break;
    case 5:
        ui->dA_PID_get_pushButton->click();
        break;
    case 6:
        ui->Pitch2dT_PDI_get_pushButton->click();
        break;
    default:
         refreshTimerGet->stop();
        break;


    }

    counterRefreshGet++;
    valuesMutex.unlock();

}

void SlugsPIDControl::slugsSetGeneral()
{
    valuesMutex.lock();
    switch(counterRefreshSet)
    {
    case 1:
        ui->dT_PID_set_pushButton->click();
        break;
    case 2:
        ui->HELPComm_PDI_set_pushButton->click();
        break;
    case 3:
        ui->dE_PID_set_pushButton->click();
        break;
    case 4:
        ui->dR_PDI_set_pushButton->click();
        break;
    case 5:
        ui->dA_PID_set_pushButton->click();
        break;
    case 6:
        ui->Pitch2dT_PDI_set_pushButton->click();
        break;
    default:
        refreshTimerSet->stop();
        break;

    }

    counterRefreshSet++;
    valuesMutex.unlock();
}


void SlugsPIDControl::slugsTimerStartSet()
{
    counterRefreshSet = 1;
    refreshTimerSet->start();

}

void SlugsPIDControl::slugsTimerStartGet()
{
    counterRefreshGet = 1;
    refreshTimerGet->start();

}
void SlugsPIDControl::slugsTimerStop()
{
//    refreshTimerGet->stop();
//     counterRefresh = 1;

}
