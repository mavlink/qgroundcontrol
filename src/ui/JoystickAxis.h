#ifndef JOYSTICKAXIS_H
#define JOYSTICKAXIS_H

#include <QWidget>
#include "JoystickInput.h"

namespace Ui {
class JoystickAxis;
}

class JoystickAxis : public QWidget
{
    Q_OBJECT
    
public:
    explicit JoystickAxis(int id, QWidget *parent = 0);
    ~JoystickAxis();

signals:
    /** @brief Signal a change in this axis' yaw/pitch/roll mapping */
    void mappingChanged(int id, JoystickInput::JOYSTICK_INPUT_MAPPING newMapping);
    /** @brief Signal a change in this axis' inversion status */
    void inversionChanged(int id, bool);
    /** @brief Signal a change in this axis' range limit */
    void rangeLimitChanged(int id, bool);

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
    /** @brief Emit signal when the inversion checkbox is changed. */
    void inversionCheckBoxChanged(bool inverted);
    /** @brief Emit signal when the limit range checkbox is changed. */
    void limitRangeCheckBoxChanged(bool limited);
};

#endif // JOYSTICKAXIS_H
