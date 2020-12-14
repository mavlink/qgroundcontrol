#include "login.h"
#include "customerdata.h"

Login::Login(QObject *parent): QObject(parent){
    mutex = false;
    myURL = "https://drone-management-api-ankit1998.herokuapp.com/customer";
}

QString Login::userName(){
    return m_userName;
}

QString Login::otp(){
    return m_otp;
}

QString Login::passWord(){
    return m_passWord;
}

bool Login::okButton(){
    return checkNow;
}

bool Login::otpButton(){
    return m_otpButton;
}

//drone number value
QString Login::droneNo(){
    return m_droneNo;
}

void Login::setUserName(const QString &userName){
    if (userName == m_userName)
        return;
    m_userName = userName;
    emit userNameChanged();
}

void Login::setOTP(const QString &otp)
{
    if (otp == m_otp)
        return;
    m_otp = otp;
    emit otpChanged();
}
void Login::setPassWord(const QString &passWord){
    if (passWord == m_passWord)
        return;

    m_passWord = passWord;
    emit passWordChanged();
}

// setting drone number value
void Login::setDroneNo(const QString &droneNo){
    if(droneNo != "123456354")
    {
        m_droneNo = "123456354";
       /* QByteArray n;
        n.append("{\"droneNo\":\"");
        n.append(m_droneNo);
        n.append("\"}");
        postDroneNo(myURL+"/checkMyDrone",n);*/
     }
    emit droneNoChanged();

}

void Login::checkDataBase(bool okButton)
{
    if(okButton){
        if(mutex){
            //customer already logged in
        }
        else{
            // Tries to log in
            QByteArray jsonString;                      // pack data
            jsonString.append("{\"email\":\"");
            jsonString.append(m_userName);
            jsonString.append("\",\"password\":\"");
            jsonString.append(m_passWord);
            jsonString.append("\"}");
            getInstance()->postEmailPass(myURL+"/login", jsonString);
        }
        emit verifyCredentials();
    }
}

void Login::checkOTP(bool otpButton)
{
    if(otpButton){
        if(mutex){
            //customer already logged in
        }
        else{
            QByteArray s;
            s.append("{\"otp\":\"");
            s.append(m_otp);
            s.append("\"}");
            getInstance()->postOTP(myURL+"/otpValidation", s);

        }
        emit verifyOTP();
    }
}
