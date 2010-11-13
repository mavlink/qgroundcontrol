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

public slots:

    void setRedColorStyle();
    void setGreenColorStyle();

    void changeColor_AirSpeed_groupBox();

    void connectButtons();
    void connect_AirSpeed_LineEdit();

private:
    Ui::SlugsPIDControl *ui;
    bool change_dT;

    //Color Styles
    QString REDcolorStyle;
    QString GREENcolorStyle;
    QString ORIGINcolorStyle;
};

#endif // SLUGSPIDCONTROL_H
