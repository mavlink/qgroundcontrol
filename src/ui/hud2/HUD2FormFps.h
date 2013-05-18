#ifndef HUD2FPSFORM_H
#define HUD2FPSFORM_H

#include <QWidget>
#include "HUD2IndicatorFps.h"

namespace Ui {
class HUD2FormFps;
}

class HUD2FormFps : public QWidget
{
    Q_OBJECT
    
public:
    explicit HUD2FormFps(HUD2IndicatorFps *fps, QWidget *parent);
    ~HUD2FormFps();

private slots:
    void on_checkBox_toggled(bool checked);

private:
    Ui::HUD2FormFps *ui;
    HUD2IndicatorFps *fps;
};

#endif // HUD2FPSFORM_H
