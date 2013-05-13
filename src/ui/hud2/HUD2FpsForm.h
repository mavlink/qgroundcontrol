#ifndef HUD2FPSFORM_H
#define HUD2FPSFORM_H

#include <QWidget>
#include "HUD2IndicatorFps.h"

namespace Ui {
class HUD2FpsForm;
}

class HUD2FpsForm : public QWidget
{
    Q_OBJECT
    
public:
    explicit HUD2FpsForm(HUD2IndicatorFps *fps, QWidget *parent);
    ~HUD2FpsForm();

private slots:
    void on_checkBox_toggled(bool checked);

private:
    Ui::HUD2FpsForm *ui;
    HUD2IndicatorFps *fps;
};

#endif // HUD2FPSFORM_H
