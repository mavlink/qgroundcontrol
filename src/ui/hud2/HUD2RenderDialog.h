#ifndef HUD2RENDERDIALOG_H
#define HUD2RENDERDIALOG_H

#include <QDialog>

namespace Ui {
class HUD2RenderDialog;
}

class HUD2RenderDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit HUD2RenderDialog(QWidget *parent = 0);
    ~HUD2RenderDialog();
    
private:
    Ui::HUD2RenderDialog *ui;
};

#endif // HUD2RENDERDIALOG_H
