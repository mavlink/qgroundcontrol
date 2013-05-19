#ifndef HUD2HORIZONFORM_H
#define HUD2HORIZONFORM_H

#include <QWidget>
#include "HUD2IndicatorHorizon.h"

namespace Ui {
class HUD2FormHorizon;
}

class HUD2FormHorizon : public QWidget
{
    Q_OBJECT
    
public:
    explicit HUD2FormHorizon(HUD2IndicatorHorizon *horizon, QWidget *parent);
    ~HUD2FormHorizon();

private:
    Ui::HUD2FormHorizon *ui;
    HUD2IndicatorHorizon *horizon;
};

#endif // HUD2HORIZONFORM_H
