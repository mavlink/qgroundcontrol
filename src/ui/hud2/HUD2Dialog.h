#ifndef HUD2DIALOG_H
#define HUD2DIALOG_H

#include <QDialog>

namespace Ui {
class HUD2Dialog;
}

class HUD2Dialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit HUD2Dialog(QWidget *parent = 0);
    ~HUD2Dialog();
    
private:
    Ui::HUD2Dialog *ui;
};

#endif // HUD2DIALOG_H
