#ifndef HUD2COLORDIALOG_H
#define HUD2COLORDIALOG_H

#include <QDialog>

#include "HUD2IndicatorHorizon.h"
#include "HUD2IndicatorFps.h"
#include "HUD2IndicatorRoll.h"
#include "HUD2IndicatorSpeed.h"
#include "HUD2IndicatorClimb.h"
#include "HUD2IndicatorCompass.h"

namespace Ui {
class HUD2ColorDialog;
}

class HUD2ColorDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit HUD2ColorDialog(HUD2IndicatorHorizon *horizon,
                             HUD2IndicatorRoll *roll,
                             HUD2IndicatorSpeed *speed,
                             HUD2IndicatorClimb *climb,
                             HUD2IndicatorCompass *compass,
                             HUD2IndicatorFps *fps,
                             QWidget *parent = 0);
    ~HUD2ColorDialog();
    
private slots:
    void on_instrumentsButton_clicked();

    void on_skyButton_clicked();

    void on_gndButton_clicked();

    void on_defaultsButton_clicked();

private:
    HUD2IndicatorHorizon *horizon;
    HUD2IndicatorRoll *roll;
    HUD2IndicatorSpeed *speed;
    HUD2IndicatorClimb *climb;
    HUD2IndicatorCompass *compass;
    HUD2IndicatorFps *fps;
    Ui::HUD2ColorDialog *ui;
};

#endif // HUD2COLORDIALOG_H
