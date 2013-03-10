#ifndef HUD2INSTRUMENTDIALOG_H
#define HUD2INSTRUMENTDIALOG_H

#include <QDialog>

namespace Ui {
class HUD2InstrumentDialog;
}

class HUD2InstrumentDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit HUD2InstrumentDialog(QWidget *parent = 0);
    ~HUD2InstrumentDialog();
    
private:
    Ui::HUD2InstrumentDialog *ui;
};

#endif // HUD2INSTRUMENTDIALOG_H
