#ifndef HUD2HORIZONFORM_H
#define HUD2HORIZONFORM_H

#include <QWidget>
#include "ui_HUD2FormHorizon.h"

namespace Ui {
class HUD2FormHorizon;
}

class HUD2FormHorizon : public QWidget
{
    Q_OBJECT
    
public:
    explicit HUD2FormHorizon(QWidget *parent);
    ~HUD2FormHorizon();
    Ui::HUD2FormHorizon *ui;

private:

};

#endif // HUD2HORIZONFORM_H
