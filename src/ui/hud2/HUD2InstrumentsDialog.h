#ifndef HUD2INSTRUMENTSDIALOG_H
#define HUD2INSTRUMENTSDIALOG_H

#include <QDialog>
#include "HUD2Ribbon.h"

#include "HUD2RibbonForm.h"
#include "HUD2FpsForm.h"
#include "HUD2HorizonForm.h"

#include "HUD2IndicatorHorizon.h"
#include "HUD2IndicatorFps.h"
#include "HUD2IndicatorRoll.h"
#include "HUD2Ribbon.h"

namespace Ui {
class HUD2InstrumentsDialog;
}

class HUD2InstrumentsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit HUD2InstrumentsDialog(HUD2IndicatorHorizon *horizon,
                                   HUD2IndicatorRoll *roll,
                                   HUD2Ribbon *speedometer,
                                   HUD2Ribbon *altimeter,
                                   HUD2Ribbon *compass,
                                   HUD2IndicatorFps *fps,
                                   QWidget *parent);
    ~HUD2InstrumentsDialog();
    
private:
    Ui::HUD2InstrumentsDialog *ui;
};

#endif // HUD2INSTRUMENTSDIALOG_H
