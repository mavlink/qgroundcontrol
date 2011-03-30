#include "OpalRT.h"

namespace OpalRT
{
//    lastErrorMsg = QString();
void OpalErrorMsg::displayLastErrorMsg()
{
    static QString lastErrorMsg;
    setLastErrorMsg();
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(lastErrorMsg);
    msgBox.exec();
}

void OpalErrorMsg::setLastErrorMsg()
{
    char* buf = new char[512];
    unsigned short len;
    static QString lastErrorMsg;
    OpalGetLastErrMsg(buf, sizeof(buf), &len);
    lastErrorMsg.clear();
    lastErrorMsg.append(buf);
    delete buf;
}
}
