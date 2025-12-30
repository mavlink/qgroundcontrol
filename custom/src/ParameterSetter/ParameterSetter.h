#pragma once 

#include <QtCore/QObject>
class ParameterSetter: public QObject{
    Q_OBJECT
public:
    ParameterSetter(QObject* parent =nullptr):QObject(parent){};
    Q_INVOKABLE QString getParameter(int compId,QString paramName);
    Q_INVOKABLE void setParameter(int compId,QString paramName,float value);
};
