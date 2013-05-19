#ifndef HUD2INSTRUMENTSDIALOG_H
#define HUD2INSTRUMENTSDIALOG_H

#include <QDialog>
#include "HUD2Ribbon.h"

#include "HUD2FormRibbon.h"
#include "HUD2FormFps.h"
#include "HUD2FormHorizon.h"

#include "HUD2IndicatorHorizon.h"
#include "HUD2IndicatorFps.h"
#include "HUD2IndicatorRoll.h"
#include "HUD2Ribbon.h"

namespace Ui {
class HUD2DialogInstruments;
}

class HUD2DialogInstruments : public QDialog
{
    Q_OBJECT
    
public:
    explicit HUD2DialogInstruments(HUD2FormHorizon *horizon_form,
                                   HUD2IndicatorRoll *roll,
                                   HUD2Ribbon *speedometer,
                                   HUD2Ribbon *altimeter,
                                   HUD2Ribbon *compass,
                                   HUD2IndicatorFps *fps,
                                   QWidget *parent);
    ~HUD2DialogInstruments();

private slots:

private:
    Ui::HUD2DialogInstruments *ui;
};

#endif // HUD2INSTRUMENTSDIALOG_H
