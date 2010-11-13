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


    void changeColor_RED_AirSpeed_groupBox(QString text);
    void changeColor_GREEN_AirSpeed_groupBox();
    /**
     * @brief Connects the SIGNALS from the editline to SLOT changeColor_RED_AirSpeed_groupBox()
     *
     * @param
     */
    void connect_AirSpeed_LineEdit();





    void connect_set_pushButtons();

private:
    Ui::SlugsPIDControl *ui;
    bool change_dT;

    //Color Styles
    QString REDcolorStyle;
    QString GREENcolorStyle;
    QString ORIGINcolorStyle;
};

#endif // SLUGSPIDCONTROL_H
