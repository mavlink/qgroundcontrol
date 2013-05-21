#ifndef HUD2RIBBONFORM_H
#define HUD2RIBBONFORM_H

#include <QWidget>
#include "HUD2Ribbon.h"
#include "HUD2Data.h"

namespace Ui {
class HUD2FormRibbon;
}

class HUD2FormRibbon : public QWidget
{
    Q_OBJECT
    
public:
    explicit HUD2FormRibbon(HUD2Ribbon *ribbon, QWidget *parent);
    ~HUD2FormRibbon();
    
private slots:
    void on_comboBox_activated(int index);

protected:
    Ui::HUD2FormRibbon *ui;
};

#endif // HUD2RIBBONFORM_H
