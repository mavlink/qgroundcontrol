#include "QGCParamID.h"
using namespace OpalRT;

QGCParamID::QGCParamID(const char *paramid):QString(paramid)
{
}

QGCParamID::QGCParamID(const QGCParamID &other):QString(other)
{

}

/*
bool QGCParamID::operator<(const QGCParamID& other)
{
    const QString *lefthand, *righthand;
    lefthand = this;
    righthand = &other;
    return lefthand < righthand;
}
*/
