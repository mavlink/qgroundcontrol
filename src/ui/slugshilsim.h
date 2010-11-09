#ifndef SLUGSHILSIM_H
#define SLUGSHILSIM_H

#include <QWidget>

namespace Ui {
    class SlugsHilSim;
}

class SlugsHilSim : public QWidget
{
    Q_OBJECT

public:
    explicit SlugsHilSim(QWidget *parent = 0);
    ~SlugsHilSim();

private:
    Ui::SlugsHilSim *ui;
};

#endif // SLUGSHILSIM_H
