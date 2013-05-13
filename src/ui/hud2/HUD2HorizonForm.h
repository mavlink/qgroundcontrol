#ifndef HUD2HORIZONFORM_H
#define HUD2HORIZONFORM_H

#include <QWidget>
#include "HUD2IndicatorHorizon.h"

namespace Ui {
class HUD2HorizonForm;
}

class HUD2HorizonForm : public QWidget
{
    Q_OBJECT
    
public:
    explicit HUD2HorizonForm(HUD2IndicatorHorizon *horizon, QWidget *parent);
    ~HUD2HorizonForm();
    
private slots:
    void on_checkBox_toggled(bool checked);

private:
    Ui::HUD2HorizonForm *ui;
    HUD2IndicatorHorizon *horizon;
};

#endif // HUD2HORIZONFORM_H
