#ifndef JOYSTICKBUTTON_H
#define JOYSTICKBUTTON_H

#include <QWidget>

#include "UASInterface.h"

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

public slots:
    /** @brief Specify the UAS that this axis should track for displaying throttle properly. */
    void setActiveUAS(UASInterface* uas);

signals:
    /** @brief Signal a change in this buttons' action mapping */
    void actionChanged(int id, int index);

private:
    int id;
    Ui::JoystickButton *m_ui;

private slots:
    void actionComboBoxChanged(int index);
};
#endif // JOYSTICKBUTTON_H
