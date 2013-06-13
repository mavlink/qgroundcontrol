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

    /**
     * @brief The JOYSTICK_MAPPING enum storing the values for each item in the mapping combobox.
     * This should match the order of items in the mapping combobox in JoystickAxis.ui.
     */
    enum JOYSTICK_AXIS_MAPPING
    {
        JOYSTICK_AXIS_MAPPING_NONE     = 0,
        JOYSTICK_AXIS_MAPPING_YAW      = 1,
        JOYSTICK_AXIS_MAPPING_PITCH    = 2,
        JOYSTICK_AXIS_MAPPING_ROLL     = 3,
        JOYSTICK_AXIS_MAPPING_THROTTLE = 4
    };

signals:
    /** @brief Signal a change in this axis' yaw/pitch/roll mapping */
    void mappingChanged(int id, int newMapping);

public slots:
    /** @brief Update the displayed value of the included progressbar.
     * @param value A value between -1.0 and 1.0.
     */
    void setValue(float value);
    
private:
    int id; ///< The ID for this axis. Corresponds to the IDs used by JoystickInput.
    Ui::JoystickAxis *ui;

private slots:
    /** @brief Handle changes to the mapping dropdown bar. */
    void mappingComboBoxChanged(int newMapping);
};

#endif // JOYSTICKAXIS_H
