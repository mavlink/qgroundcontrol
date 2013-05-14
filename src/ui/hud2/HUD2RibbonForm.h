#ifndef HUD2RIBBONFORM_H
#define HUD2RIBBONFORM_H

#include <QWidget>
#include "HUD2Ribbon.h"
#include "HUD2Data.h"

namespace Ui {
class HUD2RibbonForm;
}

class HUD2RibbonForm : public QWidget
{
    Q_OBJECT
    
public:
    explicit HUD2RibbonForm(HUD2Ribbon *ribbon, QWidget *parent);
    ~HUD2RibbonForm();
    
private slots:
    void on_comboBox_activated(int index);

private:
    Ui::HUD2RibbonForm *ui;
    HUD2Ribbon *ribbon;
};

#endif // HUD2RIBBONFORM_H
