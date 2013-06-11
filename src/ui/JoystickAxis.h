#ifndef JOYSTICKAXIS_H
#define JOYSTICKAXIS_H

#include <QWidget>

namespace Ui {
class JoystickAxis;
}

class JoystickAxis : public QWidget
{
    Q_OBJECT
    
public:
    explicit JoystickAxis(int id, QWidget *parent = 0);
    ~JoystickAxis();

public slots:
    void setValue(int value);
    
private:
    Ui::JoystickAxis *ui;
};

#endif // JOYSTICKAXIS_H
