#ifndef SLUGSPIDCONTROL_H
#define SLUGSPIDCONTROL_H

#include <QWidget>
#include<QGroupBox>
#include "UASInterface.h"
#include "QGCMAVLink.h"
#include "SlugsMAV.h"
#include "mavlink.h"
#include <QTimer>
#include <QMutex>

namespace Ui {
    class SlugsPIDControl;
}

class SlugsPIDControl : public QWidget
{
    Q_OBJECT

public:
    explicit SlugsPIDControl(QWidget *parent = 0);
    ~SlugsPIDControl();

public slots:

    /**
     * @brief Called when the a new UAS is set to active.
     *
     * Called when the a new UAS is set to active.
     *
     * @param uas The new active UAS
     */
    void activeUasSet(UASInterface* uas);

    /**
     */
    void setRedColorStyle();
    /**
     * @brief Set color StyleSheet GREEN
     *
     * @param
     */
    void setGreenColorStyle();

    /**
     * @brief Connect Set pushButtons to change the color GroupBox
     *
     * @param
     */
    void connect_set_pushButtons();

    /**
     * @brief Connect Set pushButtons to change the color GroupBox
     *
     * @param
     */
    void connect_get_pushButtons();

    /**
     * @brief Connect Edition Lines for PID Values
     *
     * @param
     */
    void connect_editLinesPDIvalues();

    /**
     * @brief send a PDI request message to UAS
     *
     * @param
     */
    void sendMessagePIDStatus(int PIDtype);

// Fuctions for Air Speed GroupBox
    /**
     * @brief Change color style to red when PID values of Air Speed are edited
     *
     *
     * @param
     */
    void changeColor_RED_AirSpeed_groupBox(QString text);
    /**
     * @brief Change color style to green when PID values of Air Speed are send to UAS
     *
     * @param
     */
    void changeColor_GREEN_AirSpeed_groupBox();
    /**
     * @brief Connects the SIGNALS from the editline to SLOT changeColor_RED_AirSpeed_groupBox()
     *
     * @param
     */
    void connect_AirSpeed_LineEdit();
    /**
     * @brief get message PID Air Speed(loop index = 0) from UAS
     *
     * @param
     */
    void get_AirSpeed_PID();


// Functions for Pitch Followei GroupBox
    /**
     * @brief Change color style to red when PID values of Pitch Followei are edited
     *
     *
     * @param
     */
     void changeColor_RED_PitchFollowei_groupBox(QString text);
     /**
         * @brief Change color style to green when PID values of Pitch Followei are send to UAS
         *
         * @param
         */
     void changeColor_GREEN_PitchFollowei_groupBox();
     /**
         * @brief Connects the SIGNALS from the editline to SLOT PitchFlowei_groupBox
         *
         * @param
         */
     void connect_PitchFollowei_LineEdit();
     /**
      * @brief get message PID Pitch Followei(loop index = 2) from UAS
      *
      * @param
      */
     void get_PitchFollowei_PID();


     // Functions for Roll Control GroupBox
     /**
          * @brief Change color style to red when PID values of Roll Control are edited
          *
          *
          * @param
          */
     void changeColor_RED_RollControl_groupBox(QString text);
     /**
              * @brief Change color style to green when PID values of Roll Control are send to UAS
              *
              * @param
              */
     void changeColor_GREEN_RollControl_groupBox();
     /**
              * @brief Connects the SIGNALS from the editline to SLOT RollControl_groupBox
              *
              * @param
              */
     void connect_RollControl_LineEdit();
     /**
      * @brief get message PID Roll Control(loop index = 4) from UAS
      *
      * @param
      */
     void get_RollControl_PID();


     // Functions for Heigth Error GroupBox
     /**
          * @brief Change color style to red when PID values of Heigth Error are edited
          *
          *
          * @param
          */
     void changeColor_RED_HeigthError_groupBox(QString text);
     /**
              * @brief Change color style to green when PID values of Heigth Error are send to UAS
              *
              * @param
              */
     void changeColor_GREEN_HeigthError_groupBox();
     /**
              * @brief Connects the SIGNALS from the editline to SLOT HeigthError_groupBox
              *
              * @param
              */
     void connect_HeigthError_LineEdit();
     /**
      * @brief get message PID Heigth Error(loop index = 1) from UAS
      *
      * @param
      */
     void get_HeigthError_PID();

     // Functions for Yaw Damper GroupBox
     /**
          * @brief Change color style to red when PID values of Yaw Damper are edited
          *
          *
          * @param
          */
     void changeColor_RED_YawDamper_groupBox(QString text);
     /**
              * @brief Change color style to green when PID values of Yaw Damper are send to UAS
              *
              * @param
              */
     void changeColor_GREEN_YawDamper_groupBox();
     /**
              * @brief Connects the SIGNALS from the editline to SLOT YawDamper_groupBox
              *
              * @param
              */
     void connect_YawDamper_LineEdit();
     /**
      * @brief get message PID Yaw Damper(loop index = 3) from UAS
      *
      * @param
      */
     void get_YawDamper_PID();



     // Functions for Pitch to dT GroupBox
     /**
          * @brief Change color style to red when PID values of Pitch to dT are edited
          *
          *
          * @param
          */
     void changeColor_RED_Pitch2dT_groupBox(QString text);
     /**
              * @brief Change color style to green when PID values of Pitch to dT are send to UAS
              *
              * @param
              */
     void changeColor_GREEN_Pitch2dT_groupBox();
     /**
              * @brief Connects the SIGNALS from the editline to SLOT Pitch2dT_groupBox
              *
              * @param
              */
     void connect_Pitch2dT_LineEdit();
     /**
      * @brief get message PID Pitch2dT(loop index = 8) from UAS
      *
      * @param
      */
     void get_Pitch2dT_PID();

     /**
          * @brief get and updates the values on widget
     */
      void slugsGetGeneral();
      /**
           * @brief Sent all values to UAS
      */
       void slugsSetGeneral();

       void slugsTimerStartSet();
       void slugsTimerStartGet();
       void slugsTimerStop();



     //Create, send and get Messages PID
    // void createMessagePID();
#ifdef MAVLINK_ENABLED_SLUGS

     void recibeMensaje(int systemId, const mavlink_action_ack_t& action);
     void receivePidValues(int systemId, const mavlink_pid_t& pidValues);

#endif // MAVLINK_ENABLED_SLUG

private:
    Ui::SlugsPIDControl *ui;

     UASInterface* activeUAS;
     int systemId;

    bool change_dT;


    //Color Styles
    QString REDcolorStyle;
    QString GREENcolorStyle;
    QString ORIGINcolorStyle;

    //SlugsMav Message
    #ifdef MAVLINK_ENABLED_SLUGS
    mavlink_pid_t pidMessage;
    mavlink_slugs_action_t actionSlugs;
#endif

    QTimer* refreshTimerSet;      ///< The main timer, controls the update view
    QTimer* refreshTimerGet;      ///< The main timer, controls the update view
    int counterRefreshSet;
    int counterRefreshGet;
    QMutex valuesMutex;
};

#endif // SLUGSPIDCONTROL_H
