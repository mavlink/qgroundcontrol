#ifndef HUD2RIBBONFORM_H
#define HUD2RIBBONFORM_H

#include <QWidget>
#include "HUD2Ribbon.h"

namespace Ui {
class HUD2RibbonForm;
}

class HUD2RibbonForm : public QWidget
{
    Q_OBJECT
    
public:
    explicit HUD2RibbonForm(HUD2Ribbon *ribbon, QWidget *parent = 0);
    ~HUD2RibbonForm();
    
private slots:
    void on_checkBoxNeedle_toggled(bool checked);

private:
    HUD2Ribbon *ribbon;
    Ui::HUD2RibbonForm *ui;
};

#endif // HUD2RIBBONFORM_H
