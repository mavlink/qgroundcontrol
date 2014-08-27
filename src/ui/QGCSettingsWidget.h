#ifndef QGCSETTINGSWIDGET_H
#define QGCSETTINGSWIDGET_H

#include <QDialog>
#include "MainWindow.h"
#include "JoystickInput.h"

class JoystickAxis;
class JoystickButton;

namespace Ui
{
class QGCSettingsWidget;
}

class QGCSettingsWidget : public QDialog
{
    Q_OBJECT

public:
    QGCSettingsWidget(JoystickInput* joystick, QWidget *parent = 0, Qt::WindowFlags flags = Qt::Sheet);
    ~QGCSettingsWidget();

public slots:
    void styleChanged(int index);
    void lineEditFinished();
    void setDefaultStyle();
    void selectStylesheet();
    void selectCustomMode(int mode);
    void resetSettings();
    /** @brief Update the UI for a new joystick based on SDL ID. */
    void createUIForJoystick();
    /**
     * @brief Update a given axis with a new value
     * @param axis The index of the axis to update.
     * @param value The new value for the axis, [-1.0:1.0].
     * @see JoystickInput:axisValueChanged
     */
    void updateAxisValue(int axis, float value);
    /** @brief Update the UI with new values for the hat.
     *  @see JoystickInput::hatDirectionChanged
     */
    void setHat(qint8 x, qint8 y);
    /** @brief Trigger a UI change based on a button being pressed */
    void joystickButtonPressed(int key);
    /** @brief Trigger a UI change based on a button being released */
    void joystickButtonReleased(int key);
    /** Update the UI assuming the joystick has stayed the same. */
    void updateJoystickUI();

protected:
    JoystickInput* joystick;  ///< Reference to the joystick
    /** @brief a list of all button labels generated for this joystick. */
    QList<JoystickButton*> buttons;
    /** @brief a lit of all joystick axes generated for this joystick. */
    QList<JoystickAxis*> axes;
    /** @brief The color to use for button labels when their corresponding button is pressed */
    QColor buttonLabelColor;

private:
    MainWindow* mainWindow;
    Ui::QGCSettingsWidget* ui;
    bool updateStyle(QString style);
    void initJoystickUI();
};

#endif // QGCSETTINGSWIDGET_H
