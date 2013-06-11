#ifndef JOYSTICKBUTTON_H
#define JOYSTICKBUTTON_H

#include <QWidget>

namespace Ui
{
class JoystickButton;
}

class JoystickButton : public QWidget
{
    Q_OBJECT
public:
    explicit JoystickButton(int id, QWidget *parent = 0);
    virtual ~JoystickButton();
private:
    int id;
    Ui::JoystickButton *m_ui;
};
#endif // JOYSTICKBUTTON_H
