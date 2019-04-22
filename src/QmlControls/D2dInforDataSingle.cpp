#include "D2dInforDataSingle.h"
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include "Freqcalibrationmodel.h"

#include <linux/errno.h>
#include <sys/stat.h>
#include <sys/cdefs.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <QVBoxLayout>
#include <QtAndroid>
#include <QAndroidJniEnvironment>

#include<QSettings>
using namespace QtAndroid;


using namespace  std;

D2dInforDataSingle* D2dInforDataSingle::pD2dInforData = NULL;

D2dInforDataSingle::D2dInforDataSingle(QObject *parent) : QObject(parent)
{
    UlRateStr = "0";
    isCalibrateFlag = false;
    whichCalibrateFromFlag = false;
    clPWRctl = 2;
    txAntCtrl = 0;
    currentRadioState = RADIO_STATE_ON;

    localServer = new QLocalServer(this);
    connect(localServer, SIGNAL(newConnection()), this, SLOT(newLocalConnection()));
    startLocalServer();


    colorListStr.clear();
    colorListStr.append("#AFF9F900");
    colorListStr.append("#AFFFFF93");
    colorListStr.append("#CFDDFFDD");
    colorListStr.append("#AF88FF88");
    colorListStr.append("#AF00FF00");

    localSocket = new QLocalSocket(this);

    //Systemproperties.get(name)
}


void D2dInforDataSingle::newLocalConnection()
{
    QLocalSocket* socket = localServer->nextPendingConnection();
    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(dataReceived()));
    QObject::connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));

}
void D2dInforDataSingle::dataReceived()
{
    //0  /* Query frequency hopping state */ QGC_QUERY_HOPPING_STATE_TAG
    //1  /* frequency hopping Control */     QGC_FREQ_HOPPING_CTRL_TAG
    //2  /* manual frequency point Select */ QGC_FREQ_SELECT_TAG
    //3  /* calibration  button clicked   */ QGC_FREQ_NEGOTIATION_TAG
    //4  /* send calibration  cmd   */       QGC_FREQ_RESET_TAG
    //5  /* D2D link has been calibrated or not. calibrated(1) or default link(0)  */       D2D_CALIBRATION_STATE_TAG

    //6 /* uplink configurable bandwidth in "MHz */     D2D_UPLINK_BANDWIDTH_CONFIG_TAG
    //7 /* downlink configrable bandwidth in "MHz  */   D2D_DOWNLINK_BANDWIDTH_CONFIG_TAG

    /*
    //#define D2D_UL_GRANT_BANDWIDTH_TAG            "UL_BW"
    //#define D2D_UPLINK_BANDWIDTH_CONFIG_TAG       "UL_BW_CFG"
    */


    QString vTemp;
    QLocalSocket* socket = static_cast<QLocalSocket*>(sender());
    if (socket)
    {
       QTextStream stream(socket);
       vTemp = stream.readAll();

       qCritical() << "D2dInforDataSingle localServer dataReceived:" << vTemp;

       if (vTemp.contains(D2D_UL_DATA_RATE_TAG))//D2D_UL_DATA_RATE_TAG  //
       {
           QStringList tempList = vTemp.split(' ');
           UlRateStr = tempList.at(1);

           emit signalUpRate();
       }
       else if (vTemp.contains(D2D_UPLINK_BANDWIDTH_CONFIG_TAG))
       {
           QStringList tempList = vTemp.split(' ');
           QString temp = tempList.at(1);
           int index = temp.toInt();
           emit uplinkCFG(index);

       }
       else if (vTemp.contains(D2D_DOWNLINK_BANDWIDTH_CONFIG_TAG))
       {
           QStringList tempList = vTemp.split(' ');
           QString temp = tempList.at(1);


           int index = temp.toInt();
           emit downlinkCFG(index);
       }
       else if(vTemp.contains(D2D_SNR_LIST_UPDATED_TAG)) //updata
       {
           getList();
       }
       else if (vTemp.contains(D2D_FREQ_LIST_RECEIVED_TAG))//
       {
           sendCalibrationCmd(8);
           if(whichCalibrateFromFlag)
           {
               return;
           }
       }
       else if (vTemp.contains(QGC_CMD_SUCCESS_TAG)) //receive
       {
           if(sendCmdStr == "QGCFREQNEG")
           {
               return;
           }

           if(whichCalibrateFromFlag)
           {
               emit maintoolbarCalibrateSucceed();
               whichCalibrateFromFlag = false;
               return;
           }
           emit setCurrentCalibrateValueSucceed();
       }
       else if (vTemp.contains(QGC_CMD_FAIL_TAG)) //receive
       {
           if(whichCalibrateFromFlag)
           {
               emit maintoolbarCalibrateFalied();
               whichCalibrateFromFlag = false;
               return;
           }
           emit setCurrentCalibrateValueFalied();
       }
       else if (vTemp.contains(D2D_FREQ_HOPPING_STATE_TAG))
       {
           QStringList tempList = vTemp.split(' ');
           emit setCalibrateStr(tempList.at(1));
       }
       else if (vTemp.contains(D2D_CALIBRATION_STATE_TAG))
       {
           QStringList tempList = vTemp.split(' ');
           if(tempList.at(1)!= "0")
           {
               emit calibrateChecked();
           }
           else
           {
               emit calibrateNoChecked();
           }
       }
       else if (vTemp.contains(D2D_SERVICE_STATUS_TAG))
       {
           QStringList tempList = vTemp.split(' ');
           QString temp = tempList.at(1);
           int index = temp.toInt();
           if(index == 3)
           {
               emit srvStateSingle(index);
           }
           else if(index == 6)
           {
               emit srvStateSingle(index);
           }
       }
       else if (vTemp.contains(D2D_TX_POWER_CTRL_STATE_TAG))
       {
           QStringList tempList = vTemp.split(' ');
           QString temp = tempList.at(1);
           int index = temp.toInt();
           if((index == 0) || (index == 1))
           {
               emit clPWRctlSingle(index);
           }
       }
       else if (vTemp.contains(D2D_TX_ANT_BITMAP_TAG))
       {
           QStringList tempList = vTemp.split(' ');
           QString temp = tempList.at(1);
           int index = temp.toInt();
           if((index == 2) || (index == 1))
           {
               emit txAntCtrlSingle(index);
           }
       }
       else if (vTemp.contains(D2D_RADIO_STATE_TAG))
       {
           QStringList tempList = vTemp.split(' ');
           QString temp = tempList.at(1);
           int index = temp.toInt();
           if((index == RADIO_STATE_ON)&&(currentRadioState != RADIO_STATE_ON))
           {
               emit updateRadioState();
           }
           currentRadioState = index;
       }
       else
       {
           //tempStr = QString::number(datacount)  +"," + "other";
           //qCritical() << "unknown d2d info message tag, ignore" ;
       }
    }
}

D2dInforDataSingle* D2dInforDataSingle::getD2dInforData()
{
    QMutex mutex;
    if (pD2dInforData == NULL)
    {
        mutex.lock();
        pD2dInforData = new D2dInforDataSingle();
    }
    mutex.unlock();
    return pD2dInforData;
}


void D2dInforDataSingle::startLocalServer()
{
    QLocalServer::removeServer(SOCKET_NAME_D2D_INFO);
    if(!localServer->listen(SOCKET_NAME_D2D_INFO))
    {
       //qCritical() << "D2dInforDataSingle localServer listen failed.";
    }
    else
    {
       //qCritical() << "D2dInforDataSingle localServer listen OK.";
    }
}

void D2dInforDataSingle::getList()
{
    ylist.clear();
    xlist.clear();
    currentNumValue = 0;
    readFile();
    emit signalList();
}

void D2dInforDataSingle::readFile()
{
    QFile file;
    file.setFileName(D2D_INTERFERENCE_LIST_FILE_NAME);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine();
        QStringList temp = line.split(' ');
        if(temp.count() == 2)
        {
            int dataNum = temp.at(0).toInt();
            if(yMinValue > dataNum)
            {
                yMinValue = dataNum;
            }
            if(yMaxValue < dataNum)
            {
                yMaxValue = dataNum;
            }

            QString intstr = temp.at(1);
            int intvalue = intstr.toInt();

            QObject* data = new FreqCalibrationModel(getDataColorStr(intvalue));
            dataList.prepend(data);

            //cliFroTest
            ylist.append(intvalue);
            xlist.append(dataNum);

            if(ylist.count() != 1)
            {
                if(cliYminValue > intvalue)
                {
                    cliYminValue = intvalue;
                }
                if(cliYmaxValue < intvalue)
                {
                    cliYmaxValue = intvalue;
                }
                if(dataNum == currentNumValue)
                {
                    cliYcurValue = intvalue;
                }
            }
            else
            {
                cliYminValue = intvalue;
                cliYmaxValue = intvalue;
            }
        }
        else if(temp.count() == 1)
        {
            currentNumValue = temp.at(0).toInt();

            yMinValue = temp.at(0).toInt();
            yMaxValue = temp.at(0).toInt();

            currentNumList.prepend(currentNumValue);
            if(currentNumList.count() > MAX_CURRENT_NUMBER)
            {
                currentNumList.pop_back();
            }
        }
    }

    file.close();
    if(dataList.count() > MAX_DATA_NUMBER)
    {
        for(int i = 0;i < EVERYTIME_DATA_NUMBER;i++)
        {
            QObject* temptr = dataList.last();
            dataList.removeLast();
            delete temptr;
            temptr = NULL;
        }
    }

    cliYminValue--;
    cliYmaxValue++;

    //update clidata
    while(cliDataList.count() > 0)
    {
        QObject* temptr = cliDataList.at(0);
        cliDataList.removeFirst();
        delete temptr;
        temptr = NULL;
    }
    cliDataList.clear();

    for(int i = 0;i < ylist.count();i++)
    {
        QObject* data = new CliTestModel(getCliDataPercent(ylist.at(i)));
        cliDataList.append(data);
    }
}

void D2dInforDataSingle::getFrequencyList()
{
    xlist.clear();
    ylist.clear();
    currentNumValue = 0;

    QFile file;
    file.setFileName(D2D_FREQUENCY_LIST_FILE_NAME);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine();
        QStringList temp = line.split(' ');
        if(temp.count() == 2)
        {
            int dataNum = temp.at(0).toInt();
            if(yMinValue > dataNum)
            {
                yMinValue = dataNum;
            }
            if(yMaxValue < dataNum)
            {
                yMaxValue = dataNum;
            }

            QString intstr = temp.at(1);
            int intvalue = intstr.toInt();

            //cliFroTest
            ylist.append(intvalue);
            xlist.append(dataNum);

            if(ylist.count() != 1)
            {
                if(cliYminValue > intvalue)
                {
                    cliYminValue = intvalue;
                }
                if(cliYmaxValue < intvalue)
                {
                    cliYmaxValue = intvalue;
                }
                if(dataNum == currentNumValue)
                {
                    cliYcurValue = intvalue;
                }
            }
            else
            {
                cliYminValue = intvalue;
                cliYmaxValue = intvalue;
            }
        }
        else if(temp.count() == 1)
        {
            currentNumValue = temp.at(0).toInt();

            yMinValue = temp.at(0).toInt();
            yMaxValue = temp.at(0).toInt();
        }
    }

    file.close();
    cliYminValue--;
    cliYmaxValue++;

    //update clidata
    while(cliDataList.count() > 0)
    {
        QObject* temptr = cliDataList.at(0);
        cliDataList.removeFirst();
        delete temptr;
        temptr = NULL;
    }
    cliDataList.clear();

    for(int i = 0;i < ylist.count();i++)
    {
        QObject* data = new CliTestModel(getCalibrateDataPercent(ylist.at(i)));
        cliDataList.append(data);
    }
    //calibrate also update
    //emit signalCalibrateList();
}

QList<QObject*> D2dInforDataSingle::getModelList()
{
    return dataList;
}

int D2dInforDataSingle::getModelListNumber()
{
    return ylist.count();
}

QList<QObject *> D2dInforDataSingle::getCliModelList()
{
    return cliDataList;
}

float D2dInforDataSingle::getCliDataPercent(int yvalue)
{
    return (cliYmaxValue - yvalue)/(getCliDistanceValue()*1.0);
}

float D2dInforDataSingle::getCalibrateDataPercent(int yvalue)
{
    return (yvalue - cliYminValue)/(getCliDistanceValue()*1.0);
}

int D2dInforDataSingle::getCliYMinValue()
{
    return cliYminValue;
}

int D2dInforDataSingle::getCliYMaxValue()
{
    return cliYmaxValue;
}

int D2dInforDataSingle::getCliYCurValue()
{
    return cliYcurValue;
}

int D2dInforDataSingle::getDataValueNum()
{
    return xlist.count();
}

int D2dInforDataSingle::getCliDistanceValue()
{
    return  (cliYmaxValue - cliYminValue);
}

int D2dInforDataSingle::getCliCurrentNumValue()
{
    return currentNumValue;
}

void D2dInforDataSingle::setCliUlDlValue(int value,int type)
{
    sendCalibrationCmdStr = QGC_CONFIG_BW_TAG;
    if(type == 1)
    {
        if(value == 0)
        {
            sendCalibrationCmdStr = sendCalibrationCmdStr + ":" + QString::number(type) + "," + QString::number(1);
        }
        else if(value == 1)
        {
            sendCalibrationCmdStr = sendCalibrationCmdStr + ":" + QString::number(type) + "," + QString::number(10);
        }
        else if(value == 2)
        {
            sendCalibrationCmdStr = sendCalibrationCmdStr + ":" + QString::number(type) + "," + QString::number(20);
        }
    }
    else if(type == 0)
    {
        if(value == 0)
        {
            sendCalibrationCmdStr = sendCalibrationCmdStr + ":" + QString::number(type) + "," + QString::number(10);
        }
        else if(value == 1)
        {
            sendCalibrationCmdStr = sendCalibrationCmdStr + ":" + QString::number(type) + "," + QString::number(20);
        }
    }
    else
    {
        qCritical() << "D2dInforDataSingle SetCliUlDlValue type is error.";
        return;
    }

    sendCmdStr = sendCalibrationCmdStr;

    qCritical() << "D2dInforDataSingle setCliUlDlValue sendCmdStr:" << sendCmdStr;

    localSocket->connectToServer(SERVER_NAME_D2D_INFO);
    if (!localSocket->waitForConnected())
    {
       //qCritical() << "D2dInforDataSingle sendCalibrationCmd localSocket connectToServer failed.";
       return;
    }
    int length;
    int temp = htonl(sendCalibrationCmdStr.length());
    char  sendBuf[4];
    *(int*)sendBuf = temp;

    QByteArray dome2 = sendCalibrationCmdStr.toLatin1();

    QByteArray byte;
    byte.append(sendBuf[0]);
    byte.append(sendBuf[1]);
    byte.append(sendBuf[2]);
    byte.append(sendBuf[3]);

    byte.append(dome2);

    if((length=localSocket->write(byte))!= byte.count())
    {
        //qCritical() << "D2dInforDataSingle sendCalibrationCmd localSocket Send failed." ;
    }
    else
    {
        //qCritical() << "D2dInforDataSingle sendCalibrationCmd localSocket Send OK.";
    }
    localSocket->flush();
    localSocket->disconnectFromServer();

}

QList<QObject *> D2dInforDataSingle::getCalibrateModelList()
{
    return cliDataList;
}

int D2dInforDataSingle::getSNRValue(int currentValue, int index)
{
    if((currentValue <= 0) || (index < 0) )
    {
        return 0;
    }
    int offsetNum = 0;
    if(index == 0)
    {
        offsetNum = 7;
    }
    else if(index == 1)
    {
        offsetNum = 15;
    }
    else if(index == 2)
    {
        offsetNum = 25;
    }
    else if(index == 3)
    {
        offsetNum = 50;
    }
    else if(index == 4)
    {
        offsetNum = 75;
    }
    else if(index == 5)
    {
        offsetNum = 100;
    }
    int minNuM = currentValue - offsetNum;
    int mAXNuM = currentValue + offsetNum;
    int total = 0;
    int num = 0;


    for(int i = 0;i < xlist.count();i++)
    {
        if((xlist.at(i) <= mAXNuM)&&(xlist.at(i) >= minNuM))
        {
            if(i > (ylist.count() - 1))
            {
                return 0;
            }
            total+=ylist.at(i);
            num++;
        }
    }
    //if num ==0
    if(num == 0)
    {
        return 0;
    }

    return (total/num);
}

void D2dInforDataSingle::setIsCalibrateFlag(bool value)
{
    isCalibrateFlag = value;
}

bool D2dInforDataSingle::getIsCalibrateFlag()
{
    return isCalibrateFlag;
}

void D2dInforDataSingle::setWhichCalibrateFromFlag(bool value)
{
    whichCalibrateFromFlag = value;
}

bool D2dInforDataSingle::getWhichCalibrateFromFlag()
{
    return whichCalibrateFromFlag;
}

QString D2dInforDataSingle::getSendCmdStr()
{
    return sendCmdStr;
}

void D2dInforDataSingle::setCliclPWRctl(int value)
{
    clPWRctl = value;
}

void D2dInforDataSingle::setClitxAntCtrl(int value)
{
    txAntCtrl = value;
}

QString D2dInforDataSingle::getDataColorStr(int value)
{
    if(value < -5)
    {
        return colorListStr.at(0);
    }
    else if((value >= -5)&&( value < 0))
    {
        return colorListStr.at(1);
    }
    else if((value >= 0)&&( value < 10))
    {
        return colorListStr.at(2);
    }
    else if((value >= 10)&&( value < 20))
    {
        return colorListStr.at(3);
    }
    else
    {
        return colorListStr.at(4);
    }
}
void D2dInforDataSingle::sendCalibrationCmd(int index)
{
    //0  /* Query frequency hopping state */ QGC_QUERY_HOPPING_STATE_TAG
    //1  /* frequency hopping Control */     QGC_FREQ_HOPPING_CTRL_TAG
    //2  /* manual frequency point Select */ QGC_FREQ_SELECT_TAG
    //3  /* calibration  button clicked   */ QGC_FREQ_NEGOTIATION_TAG
    //4  /* send calibration  cmd   */       QGC_FREQ_RESET_TAG
    //5  /* D2D link has been calibrated or not. calibrated(1) or default link(0)  */       QGC_QUERY_CALIBRATION_STATE_TAG

    //6 /* uplink configurable bandwidth in "MHz */     QGC_QUERY_UPLINK_BW_CONFIG_TAG
    //7 /* downlink configrable bandwidth in "MHz  */   QGC_QUERY_DOWNLINK_BW_CONFIG_TAG

    // 8 /*aoto calibrate send cmd*/                    QGC_FREQ_AUTO_CALIBRATE_TAG

    //9                                                 #define QGC_TX_POWER_CTRL_TAG

    //10                                                 #define QGC_TX_ANTENNA_CTRL_TAG

    if(index == 0){
        sendCalibrationCmdStr = QGC_QUERY_HOPPING_STATE_TAG;
    }
    else if(index == 1){
        sendCalibrationCmdStr = QGC_FREQ_HOPPING_CTRL_TAG;
        sendCalibrationCmdStr = sendCalibrationCmdStr + ":" + QString::number(currentNumValue);
    }
    else if(index == 2){
        sendCalibrationCmdStr = QGC_FREQ_SELECT_TAG;
        sendCalibrationCmdStr = sendCalibrationCmdStr + ":" + QString::number(currentNumValue);
    }
    else if(index == 3){
        sendCalibrationCmdStr = QGC_FREQ_NEGOTIATION_TAG;

        qCritical() << "D2dInforDataSingle sendCalibrationCmd QGC_FREQ_NEGOTIATION_TAG send.";
    }
    else if(index == 4){
        sendCalibrationCmdStr = QGC_FREQ_RESET_TAG;
        sendCalibrationCmdStr = sendCalibrationCmdStr + ":" + QString::number(currentNumValue);

        //qCritical() << "D2dInforDataSingle sendCalibrationCmd QGC_FREQ_RESET_TAG send.";
    }
    else if(index == 5){
        sendCalibrationCmdStr = QGC_QUERY_CALIBRATION_STATE_TAG;
    }
    else if(index == 6){
        sendCalibrationCmdStr = QGC_QUERY_UPLINK_BW_CONFIG_TAG;
    }
    else if(index == 7){
        sendCalibrationCmdStr = QGC_QUERY_DOWNLINK_BW_CONFIG_TAG;
    }
    else if(index == 8){
        sendCalibrationCmdStr = QGC_FREQ_AUTO_CALIBRATE_TAG;
        qCritical() << "D2dInforDataSingle sendCalibrationCmd QGC_FREQ_AUTO_CALIBRATE_TAG send.";
    }
    else if(index == 9){
        sendCalibrationCmdStr = QGC_TX_POWER_CTRL_TAG;
        sendCalibrationCmdStr = sendCalibrationCmdStr + ":" + QString::number(clPWRctl);
    }
    else if(index == 10){
        sendCalibrationCmdStr = QGC_TX_ANTENNA_CTRL_TAG;
        sendCalibrationCmdStr = sendCalibrationCmdStr + ":" + QString::number(txAntCtrl);
    }
    else{
        return;
    }

    sendCmdStr = sendCalibrationCmdStr;

    qCritical() << "D2dInforDataSingle sendCalibrationCmd sendCmdStr:" << sendCmdStr;

    localSocket->connectToServer(SERVER_NAME_D2D_INFO);
    if (!localSocket->waitForConnected())
    {
       //qCritical() << "D2dInforDataSingle sendCalibrationCmd localSocket connectToServer failed.";
       return;
    }
    int length;
    int temp = htonl(sendCalibrationCmdStr.length());
    char  sendBuf[4];
    *(int*)sendBuf = temp;

    QByteArray dome2 = sendCalibrationCmdStr.toLatin1();

    QByteArray byte;
    byte.append(sendBuf[0]);
    byte.append(sendBuf[1]);
    byte.append(sendBuf[2]);
    byte.append(sendBuf[3]);

    byte.append(dome2);

    if((length=localSocket->write(byte))!= byte.count())
    {
        //qCritical() << "D2dInforDataSingle sendCalibrationCmd localSocket Send failed." ;
    }
    else
    {
        //qCritical() << "D2dInforDataSingle sendCalibrationCmd localSocket Send OK.";
    }
    localSocket->flush();
    localSocket->disconnectFromServer();
}


void D2dInforDataSingle::setCurrentCalibrateValue(int value)
{
    currentNumValue = value;
}

int D2dInforDataSingle::getUlRateValue()
{
    return   UlRateStr.toInt();
}


int D2dInforDataSingle::getCurrentNumValue(int index)
{
    return currentNumList.at(index);
}

int D2dInforDataSingle::getCurrentNumList()
{
    return currentNumList.count();
}

int D2dInforDataSingle::getYMinValue()
{
    return yMinValue;
}

int D2dInforDataSingle::getYMaxValue()
{
    return yMaxValue;
}
void D2dInforDataSingle::Destroy()
{
    delete pD2dInforData;
    pD2dInforData = NULL;
}
