#include "QGCParamID.h"
using namespace OpalRT;

QGCParamID::QGCParamID(const char *paramid):data(paramid)
{
}

QGCParamID::QGCParamID(const QString s):data(s)
{

}

QGCParamID::QGCParamID(const QGCParamID &other):data(other.data)
{

}

//int8_t* QGCParamID::toInt8_t()
//{
//    int8_t
//    for (int i=0; ((i < data.size()) && (i < 15)); ++i)
//
//}
