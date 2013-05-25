#ifndef HUD2COLORDIALOG_H
#define HUD2COLORDIALOG_H

#include <QDialog>

#include "HUD2IndicatorHorizon.h"
#include "HUD2IndicatorFps.h"
#include "HUD2IndicatorRoll.h"
#include "HUD2Ribbon.h"

namespace Ui {
class HUD2DialogColor;
}

class HUD2DialogColor : public QDialog
{
    Q_OBJECT
    
public:
    explicit HUD2DialogColor(HUD2IndicatorHorizon *horizon,
                             HUD2IndicatorRoll *roll,
                             HUD2Ribbon *speed,
                             HUD2Ribbon *climb,
                             HUD2Ribbon *compass,
                             HUD2IndicatorFps *fps,
                             QWidget *parent = 0);
    ~HUD2DialogColor();
    
private slots:
    void on_instrumentsButton_clicked();
    void on_skyButton_clicked();
    void on_gndButton_clicked();
    void on_defaultsButton_clicked();

private:
    HUD2IndicatorHorizon *horizon;
    HUD2IndicatorRoll *roll;
    HUD2Ribbon *speed;
    HUD2Ribbon *climb;
    HUD2Ribbon *compass;
    HUD2IndicatorFps *fps;
    Ui::HUD2DialogColor *ui;
};

#endif // HUD2COLORDIALOG_H
