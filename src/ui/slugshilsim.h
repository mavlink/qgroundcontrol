#ifndef SLUGSHILSIM_H
#define SLUGSHILSIM_H

#include <QWidget>

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

private:
    Ui::SlugsHilSim *ui;
};

#endif // SLUGSHILSIM_H
