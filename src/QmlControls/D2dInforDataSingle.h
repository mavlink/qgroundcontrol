#ifndef D2DINFORDATASINGLE_H
#define D2DINFORDATASINGLE_H

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>

/* constant definition */
#define SOCKET_NAME_D2D_INFO                  "/tmp/d2dinfo"
#define D2D_MAX_MESSAGE_BYTES                 100

/* d2d related file path */
#define D2D_FREQUENCY_LIST_FILE_NAME          "/data/pinecone/radio/d2d_frequency.list"
#define D2D_INTERFERENCE_LIST_FILE_NAME       "/data/pinecone/radio/d2d_interference.list"

// d2d info message tag definition
#define D2D_SERVICE_STATUS_TAG                "SRV_STAT"
#define D2D_SIGNAL_STRENGTH_TAG               "RSRP"
#define D2D_UL_GRANT_BANDWIDTH_TAG            "UL_BW"
#define D2D_UL_DATA_RATE_TAG                  "UL_RATE"
#define D2D_SNR_LIST_UPDATED_TAG              "SNR_LIST_UPDATED"
#define D2D_FREQ_LIST_RECEIVED_TAG            "FREQ_LIST_RECVED"
#define D2D_FREQ_HOPPING_STATE_TAG            "FREQ_HOP_STATE"
#define D2D_PINGTEST_MAX_DELAY_TAG            "PING_DELAY_MAX"
#define D2D_PINGTEST_MIN_DELAY_TAG            "PING_DELAY_MIN"
#define D2D_PINGTEST_AVERAGE_DELAY_TAG        "PING_DELAY_AVRG"
#define QGC_CMD_SUCCESS_TAG                   "QGC_CMD_SUCCESS"
#define QGC_CMD_FAIL_TAG                      "QGC_CMD_FAIL"

#define D2D_CALIBRATION_STATE_TAG             "CALIBRATE_STAT"

#define D2D_UPLINK_BANDWIDTH_CONFIG_TAG       "UL_BW_CFG"
#define D2D_DOWNLINK_BANDWIDTH_CONFIG_TAG     "DL_BW_CFG"

#define D2D_TX_POWER_CTRL_STATE_TAG           "TX_PWR_CTRL"
#define D2D_TX_ANT_BITMAP_TAG                 "TX_ANT_BITMAP"
#define D2D_RADIO_STATE_TAG                   "RADIO_STAT"


/* qgc cmd message tag definition */
#define QGC_FREQ_NEGOTIATION_TAG              "QGCFREQNEG"
#define QGC_FREQ_RESET_TAG                    "QGCFREQRST"

#define QGC_PINGTEST_TAG                      "QGCPINGTEST"
#define QGC_FREQ_HOPPING_CTRL_TAG             "QGCHOPCTRL"
#define QGC_QUERY_HOPPING_STATE_TAG           "QGCHOPSTATE"
#define QGC_FREQ_SELECT_TAG                   "QGCFREQSLCT"
#define QGC_CONFIG_BW_TAG                     "QGCCFGBW"
#define QGC_QUERY_UPLINK_BW_CONFIG_TAG        "QGCULBWCFG"
#define QGC_QUERY_DOWNLINK_BW_CONFIG_TAG      "QGCDLBWCFG"
#define QGC_QUERY_CALIBRATION_STATE_TAG       "QGCCLBRSTATE"

#define QGC_FREQ_AUTO_CALIBRATE_TAG           "QGCAUTOCLBR"

#define QGC_TX_POWER_CTRL_TAG                 "QGCTXPWRCTRL"
#define QGC_TX_ANTENNA_CTRL_TAG               "QGCTXANTCTRL"


//socket connect server
#define SERVER_NAME_D2D_INFO                  "/tmp/qgccmd"

#define MAX_DATA_NUMBER                       (116*3)
#define MAX_CURRENT_NUMBER                     3

#define EVERYTIME_DATA_NUMBER                  116
#define MAX_COLOR_NUMBER                       5
#define MAX_DATA_DISTANCE                      25


typedef enum {
    RADIO_STATE_OFF          = 0,           /* Radio explictly powered off (eg CFUN=0) */
    RADIO_STATE_UNAVAILABLE  = 1,           /* Radio unavailable (eg, resetting or not booted) */
    RADIO_STATE_ON           = 10,          /* Radio is on */
    RADIO_STATE_FREQ_SCAN    = 11           /* D2D frequency scanning mode (eg CFUN=7), manual frequency point negotiation */
} D2D_RadioState;


class D2dInforDataSingle: public QObject
{
    Q_OBJECT

protected:
    explicit D2dInforDataSingle(QObject *parent = 0);
    ~D2dInforDataSingle() {}

private:
    static D2dInforDataSingle* pD2dInforData;
    void startLocalServer();
    QLocalServer *localServer;
    QLocalSocket *localSocket;

    void getList();
    void readFile();

    //calibrate
    void getFrequencyList();
    QString getDataColorStr(int value);

    QStringList colorListStr;

    QList<QObject*>  dataList;
    QString sendCalibrationCmdStr;

    QString  UlRateStr;
    int currentNumValue;
    QList<int> currentNumList;

    int yMinValue;
    int yMaxValue;

    //cmd record
    QString  sendCmdStr;

    //for cli_test
    QList<int> ylist;
    QList<int> xlist;
    int cliYminValue;
    int cliYmaxValue;
    int cliYcurValue;
    bool isCalibrateFlag;
    bool whichCalibrateFromFlag;

    QList<QObject*>  cliDataList;

    float getCliDataPercent(int yvalue);

    //calibrate
    float getCalibrateDataPercent(int yvalue);

    //QGCTXPWRCTRL
    int clPWRctl;

    //QGCTXANTCTRL
    int txAntCtrl;

    //D2D_RadioState
    int currentRadioState;

public slots:
    void newLocalConnection();
    void dataReceived();

signals:
    void signalList();
    //signl for maintoolbar
    void signalUpRate();
    void signalUpBw();

    void setCurrentCalibrateValueSucceed();
    void setCurrentCalibrateValueFalied();
    void setCalibrateStr(QString tmpStr);
    //test
    void calibrateSucceed();

    void calibrateChecked();
    void calibrateNoChecked();

    void uplinkCFG(int index);
    void downlinkCFG(int index);

    //SRV_STAT
    void srvStateSingle(int index);

    //maintoolbar calibrate
    void maintoolbarCalibrateFalied();
    void maintoolbarCalibrateSucceed();

    //QGCTXPWRCTRL
    void clPWRctlSingle(int index);

    //QGCTXANTCTRL
    void txAntCtrlSingle(int index);

    //D2D_RadioState
    void updateRadioState();


public:
    static void Destroy();
    static D2dInforDataSingle *getD2dInforData();

    Q_INVOKABLE QList<QObject*> getModelList();
    Q_INVOKABLE int getModelListNumber();

    Q_INVOKABLE void sendCalibrationCmd(int index);
    Q_INVOKABLE void setCurrentCalibrateValue(int value);

    Q_INVOKABLE int getUlRateValue();

    Q_INVOKABLE  int getCurrentNumValue(int index);
    Q_INVOKABLE  int getCurrentNumList();

    Q_INVOKABLE  int getYMinValue();
    Q_INVOKABLE  int getYMaxValue();

    //for cli_test
    Q_INVOKABLE QList<QObject*> getCliModelList();
    Q_INVOKABLE  int getCliYMinValue();
    Q_INVOKABLE  int getCliYMaxValue();
    Q_INVOKABLE  int getCliYCurValue();

    Q_INVOKABLE  int getDataValueNum();

    Q_INVOKABLE  int getCliDistanceValue();
    Q_INVOKABLE  int getCliCurrentNumValue();

    Q_INVOKABLE  void setCliUlDlValue(int value,int type);
    Q_INVOKABLE QList<QObject*> getCalibrateModelList();

    //snr
    Q_INVOKABLE  int getSNRValue(int currentValue,int index);

    //isCalibrateFlag;
    Q_INVOKABLE void setIsCalibrateFlag(bool value);
    Q_INVOKABLE bool getIsCalibrateFlag();

    //whichCalibrateFromFlag
    Q_INVOKABLE void setWhichCalibrateFromFlag(bool value);
    Q_INVOKABLE bool getWhichCalibrateFromFlag();


    //cmd str
    Q_INVOKABLE QString getSendCmdStr();

    //QGCTXPWRCTRL
    Q_INVOKABLE  void setCliclPWRctl(int value);

    //QGCTXANTCTRL
    Q_INVOKABLE  void setClitxAntCtrl(int value);

};
#endif // D2DINFORDATASINGLE_H
