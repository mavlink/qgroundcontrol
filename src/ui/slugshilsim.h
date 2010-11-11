#ifndef SLUGSHILSIM_H
#define SLUGSHILSIM_H

#include <QWidget>
#include <QHostAddress>
#include <QUdpSocket>
#include "LinkInterface.h"


namespace Ui {
    class SlugsHilSim;
}

class SlugsHilSim : public QWidget
{
    Q_OBJECT

public:
    explicit SlugsHilSim(QWidget *parent = 0);
    ~SlugsHilSim();

protected:
    LinkInterface* hilLink;
    QHostAddress* simulinkIp;
    QUdpSocket* txSocket;
    QUdpSocket* rxSocket;

public slots:
    void linkAdded();


private:
    Ui::SlugsHilSim *ui;
};

#endif // SLUGSHILSIM_H
