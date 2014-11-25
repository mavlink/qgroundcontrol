#include "OpalRT.h"
#include "QGCMessageBox.h"

namespace OpalRT
{
//    lastErrorMsg = QString();
void OpalErrorMsg::displayLastErrorMsg()
{
    static QString lastErrorMsg;
    setLastErrorMsg();
    QGCMessageBox::critical(QString(), lastErrorMsg);
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
