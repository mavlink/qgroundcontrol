#ifndef SLUGSPIDCONTROL_H
#define SLUGSPIDCONTROL_H

#include <QWidget>
#include<QGroupBox>

namespace Ui {
    class SlugsPIDControl;
}

class SlugsPIDControl : public QWidget
{
    Q_OBJECT

public:
    explicit SlugsPIDControl(QWidget *parent = 0);
    ~SlugsPIDControl();

protected slots:

    void changeRedColor(QGroupBox* group);
    void changeGreenColor(QGroupBox* group);

    void connectButtons();

private:
    Ui::SlugsPIDControl *ui;
};

#endif // SLUGSPIDCONTROL_H
